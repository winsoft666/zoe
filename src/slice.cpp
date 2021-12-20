/*******************************************************************************
* Copyright (C) 2019 - 2023, winsoft666, <winsoft666@outlook.com>.
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

#include "slice.h"
#include <assert.h>
#include <string.h>
#include "file_util.h"
#include "curl_utils.h"
#include "curl/curl.h"
#include "options.h"
#include "string_encode.h"
#include "verbose.h"
#include "slice_manager.h"

namespace teemo {

Slice::Slice(int32_t index,
             int64_t begin,
             int64_t end,
             int64_t init_capacity,
             std::shared_ptr<SliceManager> slice_manager)
    : index_(index)
    , begin_(begin)
    , end_(end)
    , disk_cache_size_(0L)
    , disk_cache_buffer_(nullptr)
    , curl_(nullptr)
    , header_chunk_(nullptr)
    , status_(UNFETCH)
    , failed_times_(0)
    , slice_manager_(slice_manager) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  InitializeCriticalSection(&crit_);
#else
  mutex_ = PTHREAD_MUTEX_INITIALIZER;
#endif
  capacity_.store(init_capacity);
  disk_cache_capacity_.store(0L);

  assert(end_ == -1 || (end_ + 1 >= begin_ + capacity_.load()));
}

Slice::~Slice() {
  assert(!curl_);
  tryFreeDiskCacheBuffer();
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  DeleteCriticalSection(&crit_);
#endif
}

int64_t Slice::begin() const {
  return begin_;
}

int64_t Slice::end() const {
  return end_;
}

int64_t Slice::size() const {
  return (end_ - begin_);
}

int64_t Slice::capacity() const {
  return capacity_.load();
}

int64_t Slice::diskCacheSize() const {
  return disk_cache_size_;
}

int64_t Slice::diskCacheCapacity() const {
  return disk_cache_capacity_.load();
}

int32_t Slice::index() const {
  return index_;
}

void* Slice::curlHandle() {
  return curl_;
}

static size_t __SliceWriteBodyCallback(char* buffer,
                                       size_t size,
                                       size_t nitems,
                                       void* outstream) {
  Slice* pThis = (Slice*)outstream;

  size_t write_size = size * nitems;
  if (!pThis->onNewData(buffer, write_size)) {
    assert(false);
    return 0;  // cause CURLE_WRITE_ERROR
  }

  return write_size;
}

Result Slice::start(void* multi, int64_t disk_cache_size, int32_t max_speed) {
  if (!slice_manager_)
    return UNKNOWN_ERROR;

  if (!slice_manager_->options())
    return UNKNOWN_ERROR;

  status_ = DOWNLOADING;

  disk_cache_size_ = disk_cache_size;
  if (disk_cache_size_ > 0) {
    disk_cache_buffer_ = (char*)malloc((long)disk_cache_size_);
    if (!disk_cache_buffer_) {
      disk_cache_size_ = 0L;
    }
  }

  assert(curl_ == nullptr);
  assert(header_chunk_ == nullptr);

  curl_ = curl_easy_init();
  if (!curl_) {
    OutputVerbose(slice_manager_->options()->verbose_functor,
                  "[teemo] curl_easy_init failed.\n");
    tryFreeDiskCacheBuffer();
    status_ = INIT_FAILED;
    return INIT_CURL_FAILED;
  }

  curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
  utf8string redirect_url = slice_manager_->redirectUrl();
  utf8string url = slice_manager_->options()->url;
  curl_easy_setopt(
      curl_, CURLOPT_URL,
      (redirect_url.length() > 0 ? redirect_url.c_str() : url.c_str()));

  if (slice_manager_->options()->proxy.length() > 0) {
    curl_easy_setopt(curl_, CURLOPT_PROXY, slice_manager_->options()->proxy.c_str());
  }

  curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl_, CURLOPT_CAINFO, ca_path_.c_str());

  if (slice_manager_->options()->min_speed == -1) {
    curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, 0L);  // disabled
    curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, 0L);   // disabled
  }
  else {
    curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT,
                     slice_manager_->options()->min_speed);
    curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME,
                     slice_manager_->options()->min_speed_duration);
  }
  curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L);

  if (max_speed > 0) {
    curl_easy_setopt(curl_, CURLOPT_MAX_RECV_SPEED_LARGE,
                     (curl_off_t)max_speed);
  }
  curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 0L);
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, __SliceWriteBodyCallback);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);

  const HttpHeaders& headers = slice_manager_->options()->http_headers;
  if (headers.size() > 0) {
    for (const auto& it : headers) {
      utf8string headerStr = it.first + u8": " + it.second;
      header_chunk_ = curl_slist_append(header_chunk_, headerStr.c_str());
    }
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_chunk_);
  }

  if (end_ != -1) {
    char range[64] = {0};
    snprintf(range, sizeof(range), "%ld-%ld", (long)(begin_ + capacity_),
             (long)end_);
    if (strlen(range) > 0) {
      CURLcode err = curl_easy_setopt(curl_, CURLOPT_RANGE, range);
      OutputVerbose(slice_manager_->options()->verbose_functor,
                    "[teemo] CURLOPT_RANGE: %s.\n", range);
      if (err != CURLE_OK) {
        OutputVerbose(slice_manager_->options()->verbose_functor,
                      "[teemo] CURLOPT_RANGE failed: %ld(%s).\n", (long)err,
                      curl_easy_strerror(err));
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
        tryFreeDiskCacheBuffer();
        status_ = INIT_FAILED;
        return SET_CURL_OPTION_FAILED;
      }
    }
  }
  else {
    curl_off_t offset = begin_ + capacity_;
    CURLcode err = curl_easy_setopt(curl_, CURLOPT_RESUME_FROM_LARGE, offset);
    OutputVerbose(slice_manager_->options()->verbose_functor,
                  "[teemo] CURLOPT_RESUME_FROM_LARGE: %ld.\n", offset);
    if (err != CURLE_OK) {
      OutputVerbose(slice_manager_->options()->verbose_functor,
                    "[teemo] CURLOPT_RESUME_FROM_LARGE failed: %ld(%s).\n",
                    (long)err, curl_easy_strerror(err));
      curl_easy_cleanup(curl_);
      curl_ = nullptr;
      tryFreeDiskCacheBuffer();
      status_ = INIT_FAILED;
      return SET_CURL_OPTION_FAILED;
    }
  }

  CURLMcode m_code = curl_multi_add_handle(multi, curl_);
  if (m_code != CURLM_OK) {
    OutputVerbose(slice_manager_->options()->verbose_functor,
                  "[teemo] curl_multi_add_handle failed: %ld(%s).\n",
                  (long)m_code, curl_multi_strerror(m_code));
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
    tryFreeDiskCacheBuffer();
    status_ = INIT_FAILED;
    return ADD_CURL_HANDLE_FAILED;
  }

  return SUCCESSED;
}

Result Slice::stop(void* multi) {
  Result ret = SUCCESSED;
  if (curl_) {
    if (multi) {
      CURLMcode code = curl_multi_remove_handle(multi, curl_);
      if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
        OutputVerbose(slice_manager_->options()->verbose_functor,
                      "[teemo] curl_multi_remove_handle failed: %ld(%s).\n",
                      (long)code, curl_multi_strerror(code));
      }
    }
    if (header_chunk_) {
      curl_slist_free_all(header_chunk_);
      header_chunk_ = nullptr;
    }
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
  }

  if (!flushToDisk())
    ret = FLUSH_TMP_FILE_FAILED;
  tryFreeDiskCacheBuffer();

  return ret;
}

void Slice::setStatus(Slice::Status s) {
  status_ = s;
}

Slice::Status Slice::status() const {
  return status_;
}

void Slice::increaseFailedTimes() {
  failed_times_++;
}

int32_t Slice::failedTimes() const {
  return failed_times_;
}

bool Slice::isDataCompleted() {
  if (end_ == -1)
    return false;

  return ((end_ - begin_ + 1) ==
          capacity_.load() + disk_cache_capacity_.load());
}

bool Slice::flushToDisk() {
  bool bret = true;
  if (disk_cache_buffer_) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    EnterCriticalSection(&crit_);
#else
    pthread_mutex_lock(&mutex_);
#endif
    int64_t written = 0;
    int64_t need_write = disk_cache_capacity_.load();
    disk_cache_capacity_ = 0L;
    std::shared_ptr<TargetFile> target_file = slice_manager_->targetFile();
    if (target_file) {
      written = target_file->write(begin_ + capacity_.load(),
                                   disk_cache_buffer_, need_write);
    }
    std::atomic_fetch_add(&capacity_, written);
    bret = (written == need_write);
    assert(bret);
    if (!bret) {
      OutputVerbose(slice_manager_->options()->verbose_functor,
                    "[teemo] Slice[%d] flush to disk failed: %lld/%lld.\n",
                    index_, written, need_write);
    }
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    LeaveCriticalSection(&crit_);
#else
    pthread_mutex_unlock(&mutex_);
#endif
  }
  return bret;
}

void Slice::tryFreeDiskCacheBuffer() {
  if (disk_cache_buffer_) {
    free(disk_cache_buffer_);
    disk_cache_buffer_ = nullptr;
    disk_cache_size_ = 0L;
    disk_cache_capacity_.store(0L);
  }
}

bool Slice::onNewData(const char* p, long data_size) {
  bool bret = false;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  EnterCriticalSection(&crit_);
#else
  pthread_mutex_lock(&mutex_);
#endif

  do {
    if (!p || data_size <= 0) {
      bret = true;
      break;
    }

    std::shared_ptr<TargetFile> target_file = slice_manager_->targetFile();
    if (!target_file) {
      break;
    }

    // no cache buffer, directly write to file.
    if (!disk_cache_buffer_) {
      int64_t written =
          target_file->write(begin_ + capacity_.load(), p, data_size);
      std::atomic_fetch_add(&capacity_, written);

      bret = (written == data_size);
      break;
    }

    if (disk_cache_size_ - disk_cache_capacity_ >= data_size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_capacity_.load()), p,
             data_size);
      disk_cache_capacity_ += data_size;
      bret = true;
      break;
    }

    int64_t need_write = disk_cache_capacity_.load();

    disk_cache_capacity_.store(0L);

    int64_t written =
        target_file->write(begin_ + capacity_, disk_cache_buffer_, need_write);
    std::atomic_fetch_add(&capacity_, written);
    if (written != need_write) {
      bret = false;
      break;
    }

    if (disk_cache_size_ - disk_cache_capacity_ >= data_size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_capacity_.load()), p,
             data_size);
      std::atomic_fetch_add(&disk_cache_capacity_, data_size);
      bret = true;
      break;
    }

    written = target_file->write(begin_ + capacity_.load(), p, data_size);
    if (written != data_size) {
      OutputVerbose(
          slice_manager_->options()->verbose_functor,
          "[teemo] Warning: only write a part of buffer to file: %lld/%lld.\n",
          written, data_size);
    }
    std::atomic_fetch_add(&capacity_, written);

    bret = (written == data_size);
  } while (false);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  LeaveCriticalSection(&crit_);
#else
  pthread_mutex_unlock(&mutex_);
#endif

  return bret;
}
}  // namespace teemo
