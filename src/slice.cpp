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
    , status_(UNFETCH)
    , slice_manager_(slice_manager) {
  capacity_.store(init_capacity);
  disk_cache_capacity_.store(0L);
}

Slice::~Slice() {
  tryFreeDiskCacheBuffer();
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

static size_t DownloadWriteCallback(char* buffer, size_t size, size_t nitems, void* outstream) {
  Slice* pThis = (Slice*)outstream;

  size_t write_size = size * nitems;
  if (pThis->onNewData(buffer, write_size)) {
    // TODO
  }

  return write_size;
}

Result Slice::start(void* multi, int64_t disk_cache_size, int32_t max_speed) {
  status_ = DOWNLOADING;

  disk_cache_size_ = disk_cache_size;
  if (disk_cache_size_ > 0) {
    disk_cache_buffer_ = (char*)malloc((long)disk_cache_size_);
    if (!disk_cache_buffer_) {
      disk_cache_size_ = 0L;
    }
  }

  curl_ = curl_easy_init();
  if (!curl_) {
    tryFreeDiskCacheBuffer();
    status_ = DOWNLOAD_FAILED;
    return INIT_CURL_FAILED;
  }

  curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl_, CURLOPT_URL, slice_manager_->options()->url.c_str());
  curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0);
  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl_, CURLOPT_CAINFO, ca_path_.c_str());
  curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, 10L);
  curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, 30L);

  curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1);

  if (max_speed > 0) {
    curl_easy_setopt(curl_, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)max_speed);
  }

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, DownloadWriteCallback);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
  char range[64] = {0};
  snprintf(range, sizeof(range), "%ld-%ld", (long)(begin_ + capacity_), (long)end_);
  if (strlen(range) > 0) {
    CURLcode err = curl_easy_setopt(curl_, CURLOPT_RANGE, range);
    if (err != CURLE_OK) {
      curl_easy_cleanup(curl_);
      curl_ = nullptr;
      tryFreeDiskCacheBuffer();
      status_ = DOWNLOAD_FAILED;
      return SET_CURL_OPTION_FAILED;
    }
  }

  CURLMcode m_code = curl_multi_add_handle(multi, curl_);
  if (m_code != CURLE_OK) {
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
    tryFreeDiskCacheBuffer();
    status_ = DOWNLOAD_FAILED;
    return ADD_CURL_HANDLE_FAILED;
  }

  return SUCCESSED;
}

void Slice::stop(void* multi) {
  if (curl_) {
    if (multi) {
      CURLMcode code = curl_multi_remove_handle(multi, curl_);
      if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
      }
    }
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
  }

  tryFreeDiskCacheBuffer();
}

void Slice::setFetched() {
  status_ = FETCHED;
}

Slice::Status Slice::status() const {
  return status_;
}

bool Slice::isCompleted() {
  if (end_ == -1)
    return false;

  return ((end_ - begin_ + 1) == capacity_ + disk_cache_capacity_);
}

bool Slice::flushToDisk() {
  bool bret = true;
  if (disk_cache_buffer_) {
    int64_t written = 0;
    int64_t need_write = disk_cache_capacity_.load();
    disk_cache_capacity_ = 0L;
    std::shared_ptr<TargetFile> target_file = slice_manager_->targetFile();
    if (target_file) {
      written = target_file->Write(begin_ + capacity_.load(), disk_cache_buffer_, need_write);
    }
    std::atomic_fetch_add(&capacity_, written);
    bret = (written == need_write);
  }
  return bret;
}

void Slice::tryFreeDiskCacheBuffer() {
  if (disk_cache_buffer_) {
    free(disk_cache_buffer_);
    disk_cache_buffer_ = nullptr;
    disk_cache_size_ = 0L;
    disk_cache_capacity_ = 0L;
  }
}

bool Slice::onNewData(const char* p, long data_size) {
  bool bret = false;
  do {
    if (!p || data_size <= 0) {
      bret = true;
      break;
    }

    std::shared_ptr<TargetFile> target_file = slice_manager_->targetFile();
    if (!target_file) {
      break;
    }

    if (!disk_cache_buffer_) {
      int64_t written = target_file->Write(begin_ + capacity_.load(), p, data_size);
      std::atomic_fetch_add(&capacity_, written);

      bret = (written == data_size);
      break;
    }

    if (disk_cache_size_ - disk_cache_capacity_ >= data_size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_capacity_.load()), p, data_size);
      disk_cache_capacity_ += data_size;
      bret = true;
      break;
    }

    int64_t need_write = disk_cache_capacity_.load();

    disk_cache_capacity_.store(0L);

    int64_t written = target_file->Write(begin_ + capacity_, disk_cache_buffer_, need_write);
    std::atomic_fetch_add(&capacity_, written);
    if (written != need_write) {
      break;
    }

    if (disk_cache_size_ - disk_cache_capacity_ >= data_size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_capacity_.load()), p, data_size);
      std::atomic_fetch_add(&disk_cache_capacity_, data_size);
      bret = true;
      break;
    }

    written = target_file->Write(begin_ + capacity_.load(), p, data_size);
    std::atomic_fetch_add(&capacity_, written);

    bret = (written == data_size);
    break;
  } while (false);

  return bret;
}
}  // namespace teemo
