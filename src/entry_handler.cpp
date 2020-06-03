/*******************************************************************************
* Copyright (C) 2018 - 2020, winsoft666, <winsoft666@outlook.com>.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
*
* Expect bugs
*
* Please use and enjoy. Please let me know of any bugs/improvements
* that you have found/implemented and I will fix/incorporate them into this
* file.
*******************************************************************************/

#include "entry_handler.h"
#include <assert.h>
#include <functional>
#include "curl_utils.h"
#include "file_util.h"

namespace teemo {

utf8string bool2string(bool b) {
  return (b ? "true" : "false");
}

EntryHandler::EntryHandler()
    : options_(nullptr)
    , slice_manager_(nullptr)
    , progress_handler_(nullptr)
    , speed_handler_(nullptr) {
  user_stop_.store(false);
}

EntryHandler::~EntryHandler() {}

static size_t __fetchFileInfoCallback(char* buffer, size_t size, size_t nitems, void* outstream) {
  return (size * nitems);
}

std::shared_future<Result> EntryHandler::start(Options* options) {
  options_ = options;
  async_task_ = std::async(std::launch::async, std::bind(&EntryHandler::asyncTaskProcess, this));
  return async_task_;
}

void EntryHandler::stop() {
  user_stop_.store(true);
  options_->internal_stop_event.set();
}

bool EntryHandler::isDownloading() {
  if (async_task_.valid() &&
      async_task_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    return true;
  return false;
}

Result EntryHandler::asyncTaskProcess() {
  options_->internal_stop_event.unset();
  user_stop_.store(false);

  Result ret = _asyncTaskProcess();

  options_->internal_stop_event.set();

  if (speed_handler_)
    speed_handler_.reset();
  if (progress_handler_)
    progress_handler_.reset();
  if (slice_manager_)
    slice_manager_.reset();

  if (options_->result_functor)
    options_->result_functor(ret);
  return ret;
}

Result EntryHandler::_asyncTaskProcess() {
  assert(!slice_manager_.get());
  slice_manager_ = std::make_shared<SliceManager>(options_);

  if (slice_manager_->loadExistSlice() != SUCCESSED) {
    int64_t origin_file_size = 0L;
    bool fetch_size_ret = false;
    int32_t try_times = 0;
    do {
      fetch_size_ret = fetchFileInfo(origin_file_size);
      if (fetch_size_ret)
        break;
    } while (++try_times <= options_->fetch_file_info_retry);

    origin_file_size = fetch_size_ret ? origin_file_size : -1;
    slice_manager_->setOriginFileSize(origin_file_size);

    // If target file is an empty file, create it.
    //
    if (origin_file_size == 0) {
      slice_manager_.reset();
      return FileUtil::CreateFixedSizeFile(options_->target_file_path, 0)
                 ? SUCCESSED
                 : CREATE_TARGET_FILE_FAILED;
    }
  }

  Result ms_ret = slice_manager_->tryMakeSlices();
  if (ms_ret != SUCCESSED) {
    slice_manager_.reset();
    return ms_ret;
  }

  if (slice_manager_->isAllSliceCompleted()) {
    Result ret = slice_manager_->finishDownload();
    slice_manager_.reset();
    return ret;
  }

  multi_ = curl_multi_init();
  if (!multi_) {
    slice_manager_.reset();
    return INIT_CURL_MULTI_FAILED;
  }

  int32_t disk_cache_per_slice = 0L;
  int32_t max_speed_per_slice = 0L;
  calculateSliceInfo(std::min(slice_manager_->usefulSliceNum(), options_->thread_num),
                     &disk_cache_per_slice, &max_speed_per_slice);

  Result ss_ret = SUCCESSED;
  int32_t selected = 0;
  while (true) {
    if (selected >= options_->thread_num)
      break;
    std::shared_ptr<Slice> slice = slice_manager_->fetchUsefulSlice();
    if (!slice)
      break;
    ss_ret = slice->start(multi_, disk_cache_per_slice, max_speed_per_slice);
    if (ss_ret != SUCCESSED) {
      continue;
    }
    selected++;
  }

  if (selected == 0) {
    curl_multi_cleanup(multi_);
    multi_ = nullptr;
    slice_manager_.reset();
    return UNKNOWN_ERROR;
  }

  if (options_->progress_functor)
    progress_handler_ = std::make_shared<ProgressHandler>(options_, slice_manager_);
  if (options_->speed_functor)
    speed_handler_ =
        std::make_shared<SpeedHandler>(slice_manager_->totalDownloaded(), options_, slice_manager_);

  int still_running = 0;
  CURLMcode m_code = curl_multi_perform(multi_, &still_running);

  do {
    if (options_->internal_stop_event.isSetted() ||
        (options_->user_stop_event && options_->user_stop_event->isSetted()))
      break;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    long curl_timeo = -1;
    curl_multi_timeout(multi_, &curl_timeo);

    if (curl_timeo > 0) {
      timeout.tv_sec = curl_timeo / 1000;
      if (timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
    }
    else {
      timeout.tv_sec = 0;
      timeout.tv_usec = 100 * 1000;
    }

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    CURLMcode code = curl_multi_fdset(multi_, &fdread, &fdwrite, &fdexcep, &maxfd);
    if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
      if (options_->verbose_functor)
        outputVerbose("\r\ncurl_multi_fdset failed, code: " + std::to_string((int)code) + "\r\n");
      break;
    }

    /* On success the value of maxfd is guaranteed to be >= -1. We call
      select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
      no fds ready yet so we sleep 100ms, which is the minimum suggested value in the
      curl_multi_fdset() doc.
    */
    int rc;
    if (maxfd == -1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      rc = 0;
    }
    else {
      rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    }

    if (rc != -1) {
      curl_multi_perform(multi_, &still_running);
    }

    if (still_running < options_->thread_num) {
      std::shared_ptr<Slice> slice = slice_manager_->fetchUsefulSlice();
      if (slice) {
        int32_t disk_cache_per_slice = 0L;
        int32_t max_speed_per_slice = 0L;
        calculateSliceInfo(still_running + 1, &disk_cache_per_slice, &max_speed_per_slice);
        if (slice->start(multi_, disk_cache_per_slice, max_speed_per_slice) != SUCCESSED) {
        }
        if (still_running <= 0)
          still_running = 1;
      }
    }
  } while (still_running > 0);

  if (options_->verbose_functor)
    outputVerbose("\r\nstill running handles: " + std::to_string(still_running) + "\r\n");

  size_t done_thread = 0;
  CURLMsg* msg = NULL;
  int msgsInQueue;
  while ((msg = curl_multi_info_read(multi_, &msgsInQueue)) != NULL) {
    if (msg->msg == CURLMSG_DONE) {
      if (msg->data.result == CURLE_OK) {
        done_thread++;
      }
    }
  }

  if (options_->verbose_functor) {
    outputVerbose("done thread num: " + std::to_string(done_thread) + "\r\n");
  }

  Result ret = slice_manager_->finishDownload();
  slice_manager_.reset();

  if (ret == SUCCESSED)
    return SUCCESSED;

  if (user_stop_.load() || (options_->user_stop_event && options_->user_stop_event->isSetted()))
    return CANCELED;

  return ret;
}

bool EntryHandler::fetchFileInfo(int64_t& file_size) const {
  ScopedCurl scoped_curl;
  CURL* curl = scoped_curl.GetCurl();

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_URL, options_->url.c_str());
  curl_easy_setopt(curl, CURLOPT_HEADER, 1);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, options_->network_conn_timeout);

  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path_.c_str());

  // avoid libcurl failed with "Failed writing body".
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __fetchFileInfoCallback);

  CURLcode ret_code = curl_easy_perform(curl);
  if (ret_code != CURLE_OK) {
    return false;
  }

  int http_code = 0;
  ret_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (ret_code != CURLE_OK)
    return false;

  if (ret_code == CURLE_OK) {
    if (http_code != 200 &&
        // A 350 response code is sent by the server in response to a file-related command that
        // requires further commands in order for the operation to be completed
        http_code != 350) {
      return false;
    }
  }

  file_size = 0;
  ret_code = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &file_size);
  if (ret_code != CURLE_OK) {
    return false;
  }
  return true;
}

void EntryHandler::outputVerbose(const utf8string& info) {
  if (options_->verbose_functor)
    options_->verbose_functor(info);
}

void EntryHandler::calculateSliceInfo(int32_t concurrency_num,
                                      int32_t* disk_cache_per_slice,
                                      int32_t* max_speed_per_slice) {
  if (concurrency_num <= 0) {
    if (disk_cache_per_slice) {
      *disk_cache_per_slice = options_->disk_cache_size;
    }

    if (max_speed_per_slice) {
      *max_speed_per_slice = options_->max_speed;
    }
  }
  else {
    if (disk_cache_per_slice) {
      *disk_cache_per_slice = (options_->disk_cache_size / concurrency_num);
    }

    if (max_speed_per_slice) {
      *max_speed_per_slice =
          (options_->max_speed == -1 ? -1 : (options_->max_speed / concurrency_num));
    }
  }
}
}  // namespace teemo