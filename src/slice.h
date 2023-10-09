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

#ifndef ZOE_SLICE_H_
#define ZOE_SLICE_H_
#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <atomic>
#include "target_file.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#else
#include <pthread.h>
#endif

struct curl_slist;
namespace zoe {
class SliceManager;
class Slice {
 public:
  enum Status {
    UNFETCH = 0,
    FETCHED = 1,
    DOWNLOADING = 2,
    DOWNLOAD_FAILED = 3,
    DOWNLOAD_COMPLETED = 4,
    CURL_OK_BUT_COMPLETED_NOT_SURE = 5
  };

  Slice(int32_t index,
        int64_t begin,
        int64_t end,
        int64_t init_capacity,
        std::shared_ptr<SliceManager> slice_manager);
  virtual ~Slice();

  int64_t begin() const;
  int64_t end() const;
  int64_t size() const;
  int64_t capacity() const;

  int64_t diskCacheSize() const;
  int64_t diskCacheCapacity() const;

  int32_t index() const;
  void* curlHandle();

  Result start(void* multi, int64_t disk_cache_size, int64_t max_speed);
  Result stop(void* multi); // must setStatus first

  void setStatus(Slice::Status s);
  Status status() const;

  void increaseFailedTimes();
  int32_t failedTimes() const;

  // if end_ is -1, this function will return false.
  bool isDataCompletedClearly() const;

  bool onNewData(const char* p, long size);
  bool flushToDisk();
 protected:
  void freeDiskCacheBuffer();
 protected:
  int32_t index_;
  int64_t begin_; // data range is [begin_, end_]
  int64_t end_;
  std::atomic<int64_t> disk_capacity_;  // data size in disk file

  void* curl_;
  struct curl_slist* header_chunk_;

  int64_t disk_cache_size_;  // byte
  std::atomic<int64_t> disk_cache_capacity_; // data size in cache.
  char* disk_cache_buffer_;

  Status status_;
  int32_t failed_times_;

  std::shared_ptr<SliceManager> slice_manager_;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  CRITICAL_SECTION crit_;
#else
  pthread_mutex_t mutex_;
#endif
};
}  // namespace zoe
#endif  // !ZOE_SLICE_H_