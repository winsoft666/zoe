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

#ifndef TEEMO_SLICE_H__
#define TEEMO_SLICE_H__
#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <atomic>
#include "slice_manage.h"
#include "curl/curl.h"
#include "teemo/teemo.h"

namespace teemo {
class SliceManage;
class Slice {
 public:
  Slice(size_t index, std::shared_ptr<SliceManage> slice_manager);
  virtual ~Slice();

  Result Init(const utf8string& slice_file_path, long begin, long end, long capacity, long disk_cache);

  long begin() const;
  long end() const;
  long capacity() const;
  long diskCacheSize() const;
  long diskCacheCapacity() const;
  size_t index() const;
  utf8string filePath() const;

  bool InitCURL(CURLM* multi, size_t max_download_speed = 0);  // bytes per seconds
  void UnInitCURL(CURLM* multi);

  bool AppendSelfToFile(FILE* f);

  bool RemoveSliceFile();

  bool IsDownloadCompleted();

  bool OnNewData(const char* p, long size);
  bool FlushDiskCache();
 protected:
  Result GenerateSliceFilePath(size_t index, const utf8string& target_file_path, utf8string& slice_path) const;
  void addCapacity(long add);
  void addDiskCacheCapacity(long add);
 protected:
  size_t index_;
  long begin_;
  long end_;
  std::atomic<long> capacity_; // data size in disk file
  utf8string file_path_;
  FILE* file_;
  CURL* curl_;

  long disk_cache_size_; // byte
  std::atomic<long> disk_cache_buffer_capacity_;
  char* disk_cache_buffer_;

  std::shared_ptr<SliceManage> slice_manager_;
};
}  // namespace teemo

#endif