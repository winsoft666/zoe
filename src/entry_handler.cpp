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

    // If target file is an empty file, create it.
    //
    if (origin_file_size == 0) {
      slice_manager_.reset();
      return FileUtil::CreateFixedSizeFile(options_->target_file_path, 0)
                 ? SUCCESSED
                 : CREATE_TARGET_FILE_FAILED;
    }

    slice_manager_->setOriginFileSize(origin_file_size);
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
    std::shared_ptr<Slice> slice = slice_manager_->fetchUsefulSlice(false, nullptr);
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


  // https://curl.haxx.se/libcurl/c/curl_multi_fdset.html
  // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select
  // https://manpages.courier-mta.org/htmlman2/select.2.html
  //
  struct timeval select_timeout;
  select_timeout.tv_sec = 1;
  select_timeout.tv_usec = 0;

  long curl_timeo = -1;
  curl_multi_timeout(multi_, &curl_timeo);

  if (curl_timeo > 0) {
    select_timeout.tv_sec = curl_timeo / 1000;
    if (select_timeout.tv_sec > 1)
      select_timeout.tv_sec = 1;
    else
      select_timeout.tv_usec = (curl_timeo % 1000) * 1000;
  }
  else {
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 100 * 1000;
  }

  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;
  int maxfd = -1;
  int still_running = 0;

  CURLMcode mcode = curl_multi_perform(multi_, &still_running);

  do {
    if (options_->internal_stop_event.isSetted() ||
        (options_->user_stop_event && options_->user_stop_event->isSetted()))
      break;


    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);
    /*
    If no file descriptors are set by libcurl, max_fd will contain -1 when this function returns. 
    Otherwise it will contain the highest descriptor number libcurl set. 
    When libcurl returns -1 in max_fd, it is because libcurl currently does something 
    that isn't possible for your application to monitor with a socket and unfortunately 
    you can then not know exactly when the current action is completed using select(). 
    You then need to wait a while before you proceed and call curl_multi_perform anyway. 
    How long to wait? Unless curl_multi_timeout gives you a lower number, we suggest 100 milliseconds or so, 
    but you may want to test it out in your own particular conditions to find a suitable value.
    */
    mcode = curl_multi_fdset(multi_, &fdread, &fdwrite, &fdexcep, &maxfd);
    if (mcode != CURLM_CALL_MULTI_PERFORM && mcode != CURLM_OK) {
      if (options_->verbose_functor)
        outputVerbose("\r\ncurl_multi_fdset failed, code: " + std::to_string((int)mcode) + "\r\n");
      break;
    }

    if (maxfd == -1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } 
    else {
      /*
      The select function returns the total number of socket handles that are ready and contained in the fd_set structures, 
      zero if the time limit expired, or SOCKET_ERROR if an error occurred. 
      If the return value is SOCKET_ERROR, WSAGetLastError can be used to retrieve a specific error code.
      */
      int rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &select_timeout);
      if (rc == SOCKET_ERROR) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
      else {
        curl_multi_perform(multi_, &still_running);
      }
    }

    if (still_running < options_->thread_num) {
      // Get a slice that not started
      // Implied: flush the disk cache buffer of completed slice, then free buffer.
      //
      std::shared_ptr<Slice> slice = slice_manager_->fetchUsefulSlice(true, multi_);
      if (slice) {
        int32_t disk_cache_per_slice = 0L;
        int32_t max_speed_per_slice = 0L;
        calculateSliceInfo(still_running + 1, &disk_cache_per_slice, &max_speed_per_slice);
        Result start_ret = slice->start(multi_, disk_cache_per_slice, max_speed_per_slice);
        if (still_running <= 0) {
          if(start_ret == SUCCESSED)
            curl_multi_perform(multi_, &still_running);
          else
            still_running = 1;
        }
      }
    }
  } while (still_running > 0);

  Result ret = slice_manager_->finishDownload();
  slice_manager_.reset();

  if (ret == SUCCESSED)
    return ret;

  if (user_stop_.load() || (options_->user_stop_event && options_->user_stop_event->isSetted()))
    ret = CANCELED; // user cancel, ignore other failed reason

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