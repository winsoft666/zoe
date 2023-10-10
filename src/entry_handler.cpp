/*******************************************************************************
*    Copyright (C) <2019-2023>, winsoft666, <winsoft666@outlook.com>.
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "entry_handler.h"
#include <assert.h>
#include <cinttypes>
#include <functional>
#include <thread>
#include "file_util.h"
#include "string_helper.hpp"
#include "string_encode.h"
#include "verbose.h"
#include "time_meter.hpp"

#define CHECK_SETOPT2(x)                                                                                  \
  do {                                                                                                   \
    CURLcode __cc__ = (x);                                                                               \
    if (__cc__ != CURLE_OK) {                                                                            \
      OutputVerbose(options_->verbose_functor, u8"" #x " failed, return: %ld.\n", (long)__cc__); \
    }                                                                                                    \
  } while (false)

namespace zoe {

utf8string bool2string(bool b) {
  return (b ? "true" : "false");
}

EntryHandler::EntryHandler()
    : options_(nullptr)
    , slice_manager_(nullptr)
    , progress_handler_(nullptr)
    , multi_(nullptr)
    , fetch_file_info_curl_(nullptr)
    , speed_handler_(nullptr) {
  user_paused_.store(false);
  user_stopped_.store(false);
  state_.store(DownloadState::STOPPED);
}

EntryHandler::~EntryHandler() {
  if (async_task_.valid())
    async_task_.get();
}

static size_t __WriteBodyCallback(char* buffer,
                                  size_t size,
                                  size_t nitems,
                                  void* outstream) {
  return (size * nitems);
}

static size_t __WriteHeaderCallback(char* buffer,
                                    size_t size,
                                    size_t nitems,
                                    void* userdata) {
  EntryHandler::FileInfo* pFileInfo =
      static_cast<EntryHandler::FileInfo*>(userdata);
  assert(pFileInfo);
  if (!pFileInfo) {
    return -1;
  }

  size_t total = size * nitems;
  utf8string header;
  header.assign(buffer, size * nitems);

  size_t pos = header.find(": ");

  if (pos == std::string::npos) {
    return total;
  }

  utf8string key = header.substr(0, pos);
  utf8string key_lowercase = StringHelper::ToLower(key);
  utf8string value = header.substr(pos + 2, header.length() - pos - 4);

  if (key_lowercase == "content-length") {
    pFileInfo->fileSize = strtoll(value.c_str(), nullptr, 10);
  }
  else if (key_lowercase == "content-md5") {
    pFileInfo->contentMd5 = value;
  }
  else if (key_lowercase == "accept-ranges") {
    if (StringHelper::IsEqual(value, "none", true)) {
      pFileInfo->acceptRanges = false;
    }
  }

  return total;
}

std::shared_future<Result> EntryHandler::start(Options* options) {
  options_ = options;
  async_task_ = std::async(std::launch::async,
                           std::bind(&EntryHandler::asyncTaskProcess, this));
  return async_task_;
}

void EntryHandler::pause() {
  if (slice_manager_) {
    user_paused_.store(true);
    state_.store(DownloadState::PAUSED);
  }
}

void EntryHandler::resume() {
  if (slice_manager_) {
    user_paused_.store(false);
    state_.store(DownloadState::DOWNLODING);
  }
}

void EntryHandler::stop() {
  user_stopped_.store(true);
  options_->internal_stop_event.set();
  cancelFetchFileInfo();
  state_.store(DownloadState::STOPPED);
}

int64_t EntryHandler::originFileSize() const {
  if (slice_manager_)
    return slice_manager_->originFileSize();
  return -1;
}

Options* EntryHandler::options() {
  return options_;
}

DownloadState EntryHandler::state() const {
  return state_.load();
}

std::shared_future<Result> EntryHandler::futureResult() {
  return async_task_;
}

Result EntryHandler::asyncTaskProcess() {
  options_->internal_stop_event.unset();
  user_paused_.store(false);
  user_stopped_.store(false);
  state_.store(DownloadState::DOWNLODING);

  const Result ret = _asyncTaskProcess();

  options_->internal_stop_event.set();

  if (speed_handler_)
    speed_handler_.reset();

  if (progress_handler_)
    progress_handler_.reset();

  if (slice_manager_) {
    slice_manager_->cleanup();
    slice_manager_.reset();
  }

  if (options_->result_functor)
    options_->result_functor(ret);

  return ret;
}

Result EntryHandler::_asyncTaskProcess() {
  OutputVerbose(options_->verbose_functor, u8"URL: %s.\n", options_->url.c_str());
  OutputVerbose(options_->verbose_functor, u8"Thread number: %d.\n", options_->thread_num);
  OutputVerbose(options_->verbose_functor, u8"Disk Cache Size: %ld.\n", options_->disk_cache_size);
  OutputVerbose(options_->verbose_functor, u8"Target file path: %s.\n", options_->target_file_path.c_str());

  OutputVerbose(options_->verbose_functor, u8"Fetching file size...\n");
  FileInfo file_info;
  bool fetch_size_ret = false;
  int32_t try_times = 0;
  do {
    fetch_size_ret = fetchFileInfo(file_info);
    if (fetch_size_ret)
      break;
    OutputVerbose(options_->verbose_functor, u8"Fetching file size failed, retry...\n");
  } while (++try_times <= options_->fetch_file_info_retry);

  if (!fetch_size_ret) {
    OutputVerbose(options_->verbose_functor, u8"Fetch file size failed.\n");
    return FETCH_FILE_INFO_FAILED;
  }

  OutputVerbose(options_->verbose_functor, u8"File size: %" PRId64 ".\n", file_info.fileSize);

  // If target file is an empty file, create it.
  if (file_info.fileSize == 0) {
    return FileUtil::CreateFixedSizeFile(options_->target_file_path, 0)
               ? SUCCESSED
               : CREATE_TARGET_FILE_FAILED;
  }

  OutputVerbose(options_->verbose_functor, u8"Content MD5: %s.\n", file_info.contentMd5.c_str());
  OutputVerbose(options_->verbose_functor, u8"Redirect URL: %s.\n", file_info.redirect_url.c_str());

  assert(!slice_manager_);
  slice_manager_ = std::make_shared<SliceManager>(options_, file_info.redirect_url);

  if (slice_manager_->loadExistSlice(file_info.fileSize, file_info.contentMd5) != SUCCESSED) {
    slice_manager_->setOriginFileSize(file_info.fileSize);
    slice_manager_->setContentMd5(file_info.contentMd5);

    const Result ms_ret = slice_manager_->makeSlices(file_info.acceptRanges);
    if (ms_ret != SUCCESSED) {
      return ms_ret;
    }
  }

  if (slice_manager_->isAllSliceCompletedClearly(false) == SUCCESSED) {
    OutputVerbose(options_->verbose_functor, u8"All of slices have been downloaded.\n");
    return slice_manager_->finishDownloadProgress(false, multi_);
  }

  multi_ = curl_multi_init();
  if (!multi_) {
    OutputVerbose(options_->verbose_functor, u8"curl_multi_init failed.\n");
    return INIT_CURL_MULTI_FAILED;
  }

  int64_t disk_cache_per_slice = 0L;
  int64_t max_speed_per_slice = 0L;
  calculateSliceInfo(
      std::min(slice_manager_->getUnfetchAndUncompletedSliceNum(), options_->thread_num),
      &disk_cache_per_slice, &max_speed_per_slice);

  OutputVerbose(options_->verbose_functor, u8"Disk cache per slice: %" PRId64 ".\n", disk_cache_per_slice);
  OutputVerbose(options_->verbose_functor, u8"Max speed per slice: %" PRId64 ".\n", max_speed_per_slice);

  Result ss_ret = SUCCESSED;
  int32_t selected = 0;
  while (true) {
    if (selected >= options_->thread_num)
      break;

    std::shared_ptr<Slice> slice = slice_manager_->getSlice(Slice::UNFETCH);
    if (!slice)
      break;

    slice->setStatus(Slice::FETCHED);
    ss_ret = slice->start(multi_, disk_cache_per_slice, max_speed_per_slice);
    if (ss_ret != SUCCESSED) {
      OutputVerbose(options_->verbose_functor,
                    u8"Slice<%d> start downloading failed: %s.\n",
                    slice->index(), GetResultString(ss_ret));

      // fatal error, return immediately!
      curl_multi_cleanup(multi_);
      multi_ = nullptr;
      return ss_ret;
    }
    OutputVerbose(options_->verbose_functor, u8"Slice<%d> start downloading.\n", slice->index());
    selected++;
  }

  if (selected == 0) {
    OutputVerbose(options_->verbose_functor, u8"No available slice.\n");
    curl_multi_cleanup(multi_);
    multi_ = nullptr;
    return UNKNOWN_ERROR;
  }

  if (options_->progress_functor)
    progress_handler_ = std::make_shared<ProgressHandler>(options_, slice_manager_);

  if (options_->speed_functor)
    speed_handler_ = std::make_shared<SpeedHandler>(slice_manager_->totalDownloaded(), options_, slice_manager_);

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
  OutputVerbose(options_->verbose_functor, u8"Start downloading.\n");

  TimeMeter flush_time_meter;

  do {
    if (user_paused_.load()) {
      while (true) {
        if (options_->internal_stop_event.wait(50))
          break;

        if (options_->user_stop_event && options_->user_stop_event->isSetted())
          break;

        if (!user_paused_.load())
          break;
      }
    }

    if (options_->internal_stop_event.isSetted() || (options_->user_stop_event && options_->user_stop_event->isSetted()))
      break;

    if (flush_time_meter.Elapsed() >= 10000) {  // 10s
      slice_manager_->flushAllSlices();
      slice_manager_->flushIndexFile();
      flush_time_meter.Restart();
    }

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
      OutputVerbose(options_->verbose_functor,
                    u8"curl_multi_fdset failed, code: %ld(%s).\n", (long)mcode, curl_multi_strerror(mcode));
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
      const int rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &select_timeout);
      if (rc == -1) {  // SOCKET_ERROR
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }

    curl_multi_perform(multi_, &still_running);

    if (still_running < options_->thread_num) {
      updateSliceStatus();

      // Get a slice that not be fetched(of cause not completed).
      std::shared_ptr<Slice> slice = slice_manager_->getSlice(Slice::UNFETCH);
      if (!slice) {
        // Try to download the slice that is failed previous again.
        slice = slice_manager_->getSlice(Slice::DOWNLOAD_FAILED);
        if (slice) {
          if (slice->failedTimes() >= options_->slice_max_failed_times)
            slice.reset();
          else
            OutputVerbose(options_->verbose_functor, u8"Re-download slice<%d>.\n", slice->index());
        }
        else {
          if (!slice_manager_->getSlice(Slice::DOWNLOADING)) {
            // only one slice that end_ is -1, so don't need loop
            slice = slice_manager_->getSlice(Slice::CURL_OK_BUT_COMPLETED_NOT_SURE);
            if (slice) {
              if (slice_manager_->originFileSize() == -1 || slice_manager_->isAllSliceCompletedClearly(false) == SUCCESSED) {
                slice->setStatus(Slice::DOWNLOAD_COMPLETED);
                slice.reset();
              }
              else {
                OutputVerbose(options_->verbose_functor, u8"Re-download slice<%d>.\n", slice->index());
              }
            }
          }
        }
      }

      if (slice) {
        slice->setStatus(Slice::FETCHED);
        disk_cache_per_slice = 0L;
        max_speed_per_slice = 0L;
        calculateSliceInfo(still_running + 1, &disk_cache_per_slice, &max_speed_per_slice);

        const Result start_ret = slice->start(multi_, disk_cache_per_slice, max_speed_per_slice);
        if (still_running <= 0) {
          if (start_ret == SUCCESSED) {
            curl_multi_perform(multi_, &still_running);
            OutputVerbose(options_->verbose_functor, u8"Slice<%d> start downloading.\n", slice->index());
          }
          else {
            still_running = 1;
            OutputVerbose(options_->verbose_functor, u8"Slice<%d> start downloading failed: %s.\n", slice->index(), GetResultString(start_ret));
          }
        }
      }
    }
  } while (still_running > 0 || user_paused_.load());

  OutputVerbose(options_->verbose_functor, u8"Downloading end.\n");

  Result ret = slice_manager_->finishDownloadProgress(true, multi_);

  if (multi_) {
    curl_multi_cleanup(multi_);
    multi_ = nullptr;
  }

  state_.store(DownloadState::STOPPED);

  if (ret == SUCCESSED) {
    OutputVerbose(options_->verbose_functor, u8"All success!\n");
    return ret;
  }

  if (user_stopped_.load() ||
      (options_->user_stop_event && options_->user_stop_event->isSetted()))
    ret = CANCELED;  // user cancel, ignore other failed reason

  return ret;
}

bool EntryHandler::fetchFileInfo(FileInfo& fileInfo) {
  return requestFileInfo(options_->url, fileInfo);
}

bool EntryHandler::requestFileInfo(const utf8string& url, FileInfo& fileInfo) {
  if (!options_)
    return false;

  if (!fetch_file_info_curl_)
    fetch_file_info_curl_ = std::make_shared<ScopedCurl>();

  CURL* curl = fetch_file_info_curl_->GetCurl();

  curl_easy_reset(curl);
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L));
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L));
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()));
  if (options_->use_head_method_fetch_file_info)
    CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_NOBODY, 1L));
  else
    CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_NOBODY, 0L));

  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L));
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L));
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L));
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, options_->network_conn_timeout));

  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path_.c_str());

  // avoid libcurl failed with "Failed writing body".
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __WriteBodyCallback));

  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, __WriteHeaderCallback));
  CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)&fileInfo));

  if (options_->proxy.length() > 0) {
    CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_PROXY, options_->proxy.c_str()));
  }

  struct curl_slist* headerChunk = nullptr;
  const HttpHeaders& headers = options_->http_headers;
  if (headers.size() > 0) {
    for (const auto& it : headers) {
      utf8string headerStr = it.first + u8": " + it.second;
      headerChunk = curl_slist_append(headerChunk, headerStr.c_str());
    }
    CHECK_SETOPT2(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerChunk));
  }

  CURLcode ret_code = curl_easy_perform(curl);

  if (headerChunk) {
    curl_slist_free_all(headerChunk);
    headerChunk = nullptr;
  }

  if (ret_code != CURLE_OK) {
    OutputVerbose(options_->verbose_functor,
                  u8"curl_easy_perform failed, CURLcode: %ld(%s).\n",
                  (long)ret_code, curl_easy_strerror(ret_code));
    return false;
  }

  char* redirect_url = nullptr;
  if (curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirect_url) == CURLE_OK && redirect_url) {
    fileInfo.redirect_url = redirect_url;
  }

  int http_code = 0;
  if ((ret_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK) {
    OutputVerbose(
        options_->verbose_functor,
        u8"Get CURLINFO_RESPONSE_CODE failed, CURLcode: %ld(%s).\n",
        (long)ret_code, curl_easy_strerror(ret_code));
    return false;
  }

  if (http_code != 200 && http_code != 350) {
    // A 350 response code is sent by the server in response to a file-related command that
    // requires further commands in order for the operation to be completed
    OutputVerbose(options_->verbose_functor,
                  u8"HTTP response code error, code: %ld.\n",
                  (long)http_code);
    return false;
  }

  return true;
}

void EntryHandler::cancelFetchFileInfo() {
  CURL* curl = fetch_file_info_curl_->GetCurl();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1L);
  }
}

void EntryHandler::calculateSliceInfo(int32_t concurrency_num,
                                      int64_t* disk_cache_per_slice,
                                      int64_t* max_speed_per_slice) const {
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

void EntryHandler::updateSliceStatus() {
  struct CURLMsg* m = nullptr;
  do {
    int msg_in_queue = 0;
    m = curl_multi_info_read(multi_, &msg_in_queue);

    if (m && m->msg == CURLMSG_DONE) {
      const std::shared_ptr<Slice> slice = slice_manager_->getSlice(m->easy_handle);
      assert(slice);
      if (!slice)
        continue;

      if (m->data.result == CURLE_OK) {
        if (slice->isDataCompletedClearly()) {
          slice->setStatus(Slice::DOWNLOAD_COMPLETED);
          slice->stop(multi_);
        }
        else {
          if (slice->end() == -1) {
            slice->setStatus(Slice::CURL_OK_BUT_COMPLETED_NOT_SURE);
            slice->stop(multi_);
          }
          else {
            slice->setStatus(Slice::DOWNLOAD_FAILED);
            slice->increaseFailedTimes();
            slice->stop(multi_);
          }
        }
      }
      else {
        OutputVerbose(options_->verbose_functor,
                      u8"Slice<%d> download failed %ld(%s).\n",
                      slice->index(), m->data.result,
                      curl_easy_strerror(m->data.result));

        slice->setStatus(Slice::DOWNLOAD_FAILED);
        slice->increaseFailedTimes();
        slice->stop(multi_);
      }
    }
  } while (m);
}
}  // namespace zoe