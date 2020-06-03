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

#ifndef TEEMO_SLICE_MANAGE_H_
#define TEEMO_SLICE_MANAGE_H_
#pragma once

#include <vector>
#include <atomic>
#include "slice.h"
#include "teemo/teemo.h"
#include "target_file.h"

namespace teemo {
class Slice;
typedef struct _Options Options;

class SliceManager : public std::enable_shared_from_this<SliceManager> {
 public:
  SliceManager(Options* options);
  virtual ~SliceManager();

  Result loadExistSlice();
  bool flushIndexFile();

  void setOriginFileSize(int64_t file_size);
  int64_t originFileSize() const;

  std::shared_ptr<TargetFile> targetFile();

  Result tryMakeSlices();

  int64_t totalDownloaded() const;

  bool isAllSliceCompleted() const;

  Result finishDownload();

  int32_t usefulSliceNum() const;
  std::shared_ptr<Slice> fetchUsefulSlice(bool remove_completed_slice, void* mult);

  const Options* options();

  utf8string indexFilePath() const;

 protected:
  utf8string makeIndexFilePath() const;
  void dumpSlice();
 protected:
  Options* options_;
  int64_t origin_file_size_;

  utf8string index_file_path_;
  utf8string tmp_file_path_;

  std::vector<std::shared_ptr<Slice>> slices_;
  std::shared_ptr<TargetFile> target_file_;
};
}  // namespace teemo

#endif  // !TEEMO_SLICE_MANAGE_H_