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
#include "slice_manage.h"
#include "curl/curl.h"
#include "teemo.h"

namespace teemo {
class SliceManage;
class Slice {
 public:
  Slice(size_t index, std::shared_ptr<SliceManage> slice_manager);
  virtual ~Slice();

  bool Init(const utf8string& slice_file_path, long begin, long end, long capacity);

  long begin() const;
  long end() const;
  long capacity() const;
  size_t index() const;
  utf8string filePath() const;

  bool InitCURL(CURLM* multi, size_t max_download_speed = 0);  // bytes per seconds
  void UnInitCURL(CURLM* multi);

  bool AppendSelfToFile(FILE* f);

  bool RemoveSliceFile();

  FILE* GetFile();
  void IncreaseCapacity(long i);

  bool IsDownloadCompleted();

 protected:
  utf8string GenerateSliceFilePath(size_t index, const utf8string& target_file_path) const;

 protected:
  size_t index_;
  long begin_;
  long end_;
  long capacity_;
  long origin_capacity_;
  utf8string file_path_;
  FILE* file_;
  CURL* curl_;
  std::shared_ptr<SliceManage> slice_manager_;
};
}  // namespace teemo

#endif