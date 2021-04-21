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
  SliceManager(Options* options, const utf8string& redirect_url);
  virtual ~SliceManager();

  Result loadExistSlice(int64_t cur_file_size,
                        const utf8string& cur_content_md5);

  bool flushAllSlices();
  bool flushIndexFile();

  void setOriginFileSize(int64_t file_size);
  int64_t originFileSize() const;

  void setContentMd5(const utf8string& md5);
  utf8string contentMd5() const;

  std::shared_ptr<TargetFile> targetFile();

  Result makeSlices(bool accept_ranges);

  int64_t totalDownloaded() const;

  Result isAllSliceCompleted(bool need_check_hash) const;

  Result finishDownloadProgress(bool need_check_completed);

  int32_t usefulSliceNum() const;
  std::shared_ptr<Slice> fetchUsefulSlice(bool remove_completed_slice,
                                          void* mult);

  const Options* options();

  utf8string redirectUrl();

  utf8string indexFilePath() const;
 protected:
  utf8string makeIndexFilePath() const;
  void dumpSlice() const;

 protected:
  utf8string redirect_url_;
  int64_t origin_file_size_;
  utf8string content_md5_;

  utf8string index_file_path_;

  std::vector<std::shared_ptr<Slice>> slices_;
  std::shared_ptr<TargetFile> target_file_;

  Options* options_;
};
}  // namespace teemo

#endif  // !TEEMO_SLICE_MANAGE_H_