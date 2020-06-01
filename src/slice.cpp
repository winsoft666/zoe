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
#include <iostream>
#include <assert.h>
#include <string.h>
#include "file_util.h"
#include "curl_utils.h"

namespace teemo {

Slice::Slice(size_t index, std::shared_ptr<SliceManage> slice_manager)
    : index_(index)
    , slice_manager_(slice_manager)
    , begin_(0L)
    , end_(0L)
    , disk_cache_size_(0L)
    , disk_cache_buffer_(nullptr)
    , curl_(nullptr) {
  capacity_ = 0;
  disk_cache_buffer_capacity_ = 0;
}

Slice::~Slice() {
  if (disk_cache_buffer_) {
    free(disk_cache_buffer_);
    disk_cache_buffer_ = nullptr;
  }
  disk_cache_size_ = 0L;
  disk_cache_buffer_capacity_ = 0L;
}

Result Slice::Init(std::shared_ptr<TargetFile> target_file,
                   long begin,
                   long end,
                   long capacity,
                   long disk_cache) {
  begin_ = begin;
  end_ = end;
  capacity_ = capacity;
  disk_cache_size_ = disk_cache;
  target_file_ = target_file;

  if (!target_file_) {
    return CreateTmpFileFailed;
  }

  if (disk_cache_size_ > 0) {
    disk_cache_buffer_ = (char*)malloc(disk_cache_size_);
    if (!disk_cache_buffer_) {
      disk_cache_size_ = 0L;
    }
  }

  return Successed;
}

long Slice::begin() const {
  return begin_;
}

long Slice::end() const {
  return end_;
}

long Slice::capacity() const {
  return capacity_.load();
}

long Slice::diskCacheSize() const {
  return disk_cache_size_;
}

long Slice::diskCacheCapacity() const {
  return disk_cache_buffer_capacity_.load();
}

size_t Slice::index() const {
  return index_;
}

static size_t DownloadWriteCallback(char* buffer, size_t size, size_t nitems, void* outstream) {
  Slice* pThis = (Slice*)outstream;

  size_t write_size = size * nitems;
  if (pThis->OnNewData(buffer, write_size)) {
    // TODO
  }

  return write_size;
}

bool Slice::InitCURL(CURLM* multi, size_t max_download_speed /* = 0*/) {
  curl_ = curl_easy_init();

  curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl_, CURLOPT_URL, slice_manager_->GetUrl().c_str());
  curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0);
  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl_, CURLOPT_CAINFO, ca_path_.c_str());
  curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, 10L);
  curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, 30L);

  curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1);

  if (max_download_speed > 0) {
    curl_easy_setopt(curl_, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)max_download_speed);
  }

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, DownloadWriteCallback);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
  char range[64] = {0};
  if (begin_ + capacity_ >= 0 && end_ > 0 && end_ >= begin_ + capacity_)
    snprintf(range, sizeof(range), "%ld-%ld", begin_ + capacity_, end_);
  if (strlen(range) > 0) {
    CURLcode err = curl_easy_setopt(curl_, CURLOPT_RANGE, range);
    if (err != CURLE_OK) {
      std::cerr << "curl_easy_setopt CURLOPT_RANGE failed, code: " << err << std::endl;
      curl_easy_cleanup(curl_);
      curl_ = nullptr;
      return false;
    }
  }

  CURLMcode m_code = curl_multi_add_handle(multi, curl_);
  if (m_code != CURLE_OK) {
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
    return false;
  }

  return true;
}

void Slice::UnInitCURL(CURLM* multi) {
  if (curl_) {
    if (multi) {
      CURLMcode code = curl_multi_remove_handle(multi, curl_);
      if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
      }
    }
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
  }
}

bool Slice::IsDownloadCompleted() {
  if (end_ == -1)
    return false;

  return ((end_ - begin_ + 1) == capacity_);
}

bool Slice::FlushDiskCache() {
  bool bret = true;
  if (disk_cache_buffer_) {
    size_t written = 0;
    size_t need_write = disk_cache_buffer_capacity_.load();
    disk_cache_buffer_capacity_ = 0L;
    if (target_file_) {
      written = target_file_->Write(begin_ + capacity_.load(), disk_cache_buffer_, need_write);
    }
    std::atomic_fetch_add(&capacity_, written);
    bret = (written == need_write);
  }
  return bret;
}


bool Slice::OnNewData(const char* p, long size) {
  bool bret = false;
  do {
    if (!p || size <= 0) {
      bret = true;
      break;
    }

    if (!target_file_) {
      break;
    }

    if (!disk_cache_buffer_) {
      size_t written = target_file_->Write(begin_ + capacity_.load(), p, size);
      std::atomic_fetch_add(&capacity_, written);

      bret = (written == size);
      break;
    }

    if (disk_cache_size_ - disk_cache_buffer_capacity_ >= size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_buffer_capacity_.load()), p, size);
      disk_cache_buffer_capacity_ += size;
      bret = true;
      break;
    }

    size_t need_write = disk_cache_buffer_capacity_.load();

    disk_cache_buffer_capacity_.store(0L);

    size_t written = target_file_->Write(begin_ + capacity_, disk_cache_buffer_, need_write);
    std::atomic_fetch_add(&capacity_, written);
    if (written != need_write) {
      break;
    }

    if (disk_cache_size_ - disk_cache_buffer_capacity_ >= size) {
      memcpy((char*)(disk_cache_buffer_ + disk_cache_buffer_capacity_.load()), p, size);
      std::atomic_fetch_add(&disk_cache_buffer_capacity_, size);
      bret = true;
      break;
    }

    written = target_file_->Write(begin_ + capacity_.load(), p, size);
    std::atomic_fetch_add(&capacity_, written);

    bret = (written == size);
    break;
  } while (false);

  return bret;
}
}  // namespace teemo
