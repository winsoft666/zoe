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

#ifndef TEEMO_SLICE_H_
#define TEEMO_SLICE_H_
#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <atomic>
#include "slice_manager.h"
#include "target_file.h"

namespace teemo {
class SliceManager;
class Slice {
 public:
  enum Status {
    UNFETCH = 0,
    FETCHED = 1,
    DOWNLOADING = 2,
    DOWNLOAD_FAILED = 3
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

  Result start(void* multi, int64_t disk_cache_size, int32_t max_speed);
  void stop(void* multi);

  void setFetched();
  Status status() const;

  bool isCompleted();

  bool onNewData(const char* p, long size);
  bool flushToDisk();
 protected:
  void tryFreeDiskCacheBuffer();
  void outputVerbose(const utf8string& info) const;

 protected:
  int32_t index_;
  int64_t begin_;
  int64_t end_;
  std::atomic<int64_t> capacity_;  // data size in disk file

  void* curl_;

  int64_t disk_cache_size_;  // byte
  std::atomic<int64_t> disk_cache_capacity_;
  char* disk_cache_buffer_;

  Status status_;

  std::shared_ptr<SliceManager> slice_manager_;
};
}  // namespace teemo
#endif  // !TEEMO_SLICE_H_