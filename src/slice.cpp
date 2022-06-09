/*******************************************************************************
*    Copyright (C) <2019-2022>, winsoft666, <winsoft666@outlook.com>.
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

#include "slice.h"
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include "file_util.h"
#include "curl_utils.h"
#include "curl/curl.h"
#include "options.h"
#include "string_encode.h"
#include "verbose.h"
#include "slice_manager.h"

#define CHECK_SETOPT1(x)                                                                                                  \
  do {                                                                                                                    \
    CURLcode __cc__ = (x);                                                                                                \
    if (__cc__ != CURLE_OK) {                                                                                             \
      OutputVerbose(slice_manager_->options()->verbose_functor, u8"" #x " failed, return: %ld.\n", (long)__cc__); \
    }                                                                                                                     \
  } while (false)

namespace TEEMO_NAMESPACE {

Slice::Slice(int32_t index,
             int64_t begin,
             int64_t end,
             int64_t init_capacity,
             std::shared_ptr<SliceManager> slice_manager)
    : index_(index)
    , begin_(begin)
    , end_(end)
    , curl_(nullptr)
    , header_chunk_(nullptr)
    , disk_cache_size_(0L)
    , disk_cache_buffer_(nullptr)
    , status_(Slice::UNFETCH)
    , failed_times_(0)
    , slice_manager_(slice_manager) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  InitializeCriticalSection(&crit_);
#else
  mutex_ = PTHREAD_MUTEX_INITIALIZER;
#endif
  disk_capacity_.store(init_capacity);
  disk_cache_capacity_.store(0L);

  assert(end_ == -1 || (end_ + 1 >= begin_ + disk_capacity_.load()));

  if (isDataCompletedClearly())
    status_ = DOWNLOAD_COMPLETED;
}

Slice::~Slice() {
  assert(!curl_);
  freeDiskCacheBuffer();
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
  return (end_ - begin_ + 1);
}

int64_t Slice::capacity() const {
  return disk_capacity_.load();
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

Result Slice::start(void* multi, int64_t disk_cache_size, int64_t max_speed) {
  if (!slice_manager_)
    return UNKNOWN_ERROR;

  if (!slice_manager_->options())
    return UNKNOWN_ERROR;

  status_ = DOWNLOADING;

  disk_cache_size_ = disk_cache_size;
  if (disk_cache_size_ > 0) {
    // TODO: support int64_t
    disk_cache_buffer_ = (char*)malloc((size_t)disk_cache_size_);
    if (!disk_cache_buffer_) {
      disk_cache_size_ = 0L;
    }
  }

  assert(curl_ == nullptr);
  assert(header_chunk_ == nullptr);

  curl_ = curl_easy_init();
  if (!curl_) {
    OutputVerbose(slice_manager_->options()->verbose_functor, u8"curl_easy_init failed.\n");
    freeDiskCacheBuffer();
    status_ = DOWNLOAD_FAILED;
    return INIT_CURL_FAILED;
  }

  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L));
  const utf8string redirect_url = slice_manager_->redirectUrl();
  const utf8string url = slice_manager_->options()->url;
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_URL, (redirect_url.length() > 0 ? redirect_url.c_str() : url.c_str())));

  if (slice_manager_->options()->proxy.length() > 0) {
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_PROXY, slice_manager_->options()->proxy.c_str()));
  }

  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L));
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L));
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L));
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L));
  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl_, CURLOPT_CAINFO, ca_path_.c_str());

  if (slice_manager_->options()->min_speed == -1) {
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, 0L));  // disabled
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, 0L));   // disabled
  }
  else {
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, slice_manager_->options()->min_speed));
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, slice_manager_->options()->min_speed_duration));
  }
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L));

  if (max_speed > 0) {
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)max_speed));
  }
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 0L));
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, __SliceWriteBodyCallback));
  CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this));

  const HttpHeaders& headers = slice_manager_->options()->http_headers;
  if (headers.size() > 0) {
    for (const auto& it : headers) {
      utf8string headerStr = it.first + u8": " + it.second;
      header_chunk_ = curl_slist_append(header_chunk_, headerStr.c_str());
    }
    CHECK_SETOPT1(curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_chunk_));
  }

  if (end_ != -1) {
    char range[64] = {0};
    snprintf(range, sizeof(range), "%" PRId64 "-%" PRId64, begin_ + disk_capacity_, end_);
    if (strlen(range) > 0) {
      const CURLcode err = curl_easy_setopt(curl_, CURLOPT_RANGE, range);
      OutputVerbose(slice_manager_->options()->verbose_functor, u8"Slice<%d>, Range: %s.\n", index_, range);
      if (err != CURLE_OK) {
        OutputVerbose(slice_manager_->options()->verbose_functor,
                      u8"CURLOPT_RANGE failed: %ld(%s).\n", (long)err,
                      curl_easy_strerror(err));
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
        freeDiskCacheBuffer();
        status_ = DOWNLOAD_FAILED;
        return SET_CURL_OPTION_FAILED;
      }
    }
  }
  else {
    curl_off_t offset = begin_ + disk_capacity_;
    CURLcode err = curl_easy_setopt(curl_, CURLOPT_RESUME_FROM_LARGE, offset);
    OutputVerbose(slice_manager_->options()->verbose_functor,
                  u8"Slice<%d>, Range: %" PRId64 "-INFINITE.\n", index_, offset);
    if (err != CURLE_OK) {
      OutputVerbose(slice_manager_->options()->verbose_functor,
                    u8"CURLOPT_RESUME_FROM_LARGE failed: %ld(%s).\n",
                    (long)err, curl_easy_strerror(err));

      curl_easy_cleanup(curl_);
      curl_ = nullptr;

      freeDiskCacheBuffer();

      status_ = DOWNLOAD_FAILED;
      return SET_CURL_OPTION_FAILED;
    }
  }

  CURLMcode m_code = curl_multi_add_handle(multi, curl_);
  if (m_code != CURLM_OK) {
    OutputVerbose(slice_manager_->options()->verbose_functor,
                  u8"curl_multi_add_handle failed: %ld(%s).\n",
                  (long)m_code, curl_multi_strerror(m_code));
    curl_easy_cleanup(curl_);
    curl_ = nullptr;

    freeDiskCacheBuffer();

    status_ = DOWNLOAD_FAILED;
    return ADD_CURL_HANDLE_FAILED;
  }

  return SUCCESSED;
}

Result Slice::stop(void* multi) {
  Result ret = SUCCESSED;
  if (curl_) {
    if (multi) {
      const CURLMcode code = curl_multi_remove_handle(multi, curl_);
      if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
        OutputVerbose(slice_manager_->options()->verbose_functor,
                      u8"curl_multi_remove_handle failed: %ld(%s).\n",
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

  bool discard_downloaded = false;

  if (status_ == UNFETCH ||
      status_ == FETCHED ||
      status_ == DOWNLOAD_COMPLETED) {
    discard_downloaded = false;
  }
  else {
    const UncompletedSliceSavePolicy policy = slice_manager_->options()->uncompleted_slice_save_policy;
    if (policy == ALWAYS_DISCARD) {
      discard_downloaded = true;
    }
    else if (policy == SAVE_EXCEPT_FAILED) {
      discard_downloaded = (status_ == DOWNLOAD_FAILED);
    }
  }

  if (discard_downloaded) {
    disk_capacity_.store(0);
    disk_cache_capacity_.store(0);
  }
  else if (!flushToDisk()) {
    ret = FLUSH_TMP_FILE_FAILED;
  }

  freeDiskCacheBuffer();

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

bool Slice::isDataCompletedClearly() const {
  if (end_ == -1)
    return false;

  return (size() == (disk_capacity_.load() + disk_cache_capacity_.load()));
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
    const int64_t need_write = disk_cache_capacity_.load();
    disk_cache_capacity_ = 0L;

    if (need_write > 0) {
      std::shared_ptr<TargetFile> target_file = slice_manager_->targetFile();
      if (target_file) {
        written = target_file->write(begin_ + disk_capacity_.load(), disk_cache_buffer_, need_write);
      }

      std::atomic_fetch_add(&disk_capacity_, written);
      bret = (written == need_write);
      assert(bret);
      if (!bret) {
        OutputVerbose(slice_manager_->options()->verbose_functor,
                      "Slice[%d] flush to disk failed: %" PRId64 "/%" PRId64 ".\n",
                      index_, written, need_write);
      }
    }
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    LeaveCriticalSection(&crit_);
#else
    pthread_mutex_unlock(&mutex_);
#endif
  }

  return bret;
}

void Slice::freeDiskCacheBuffer() {
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
      int64_t written = target_file->write(begin_ + disk_capacity_.load(), p, data_size);
      std::atomic_fetch_add(&disk_capacity_, written);

      bret = (written == data_size);
      break;
    }

    if (disk_cache_size_ - disk_cache_capacity_ >= data_size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_capacity_.load()), p, data_size);
      disk_cache_capacity_ += data_size;
      bret = true;
      break;
    }

    const int64_t need_write = disk_cache_capacity_.load();

    disk_cache_capacity_.store(0L);

    int64_t written = target_file->write(begin_ + disk_capacity_, disk_cache_buffer_, need_write);
    std::atomic_fetch_add(&disk_capacity_, written);
    if (written != need_write) {
      bret = false;
      break;
    }

    if (disk_cache_size_ - disk_cache_capacity_ >= data_size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_capacity_.load()), p, data_size);
      std::atomic_fetch_add(&disk_cache_capacity_, data_size);
      bret = true;
      break;
    }

    written = target_file->write(begin_ + disk_capacity_.load(), p, data_size);
    if (written != data_size) {
      OutputVerbose(
          slice_manager_->options()->verbose_functor,
          u8"Warning: only write a part of buffer to file: %" PRId64 "/%" PRId64 ".\n",
          written, data_size);
    }
    std::atomic_fetch_add(&disk_capacity_, written);

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
