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

#ifndef ZOE_SLICE_MANAGE_H_
#define ZOE_SLICE_MANAGE_H_
#pragma once

#include <vector>
#include <atomic>
#include "zoe/zoe.h"
#include "target_file.h"
#include "slice.h"

namespace zoe {
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

  std::shared_ptr<TargetFile> targetFile() const;

  Result makeSlices(bool accept_ranges);

  int64_t totalDownloaded() const;

  Result isAllSliceCompletedClearly(bool try_check_hash) const;

  Result finishDownloadProgress(bool need_check_completed, void* mult);

  int32_t getUnfetchAndUncompletedSliceNum() const;
  std::shared_ptr<Slice> getSlice(Slice::Status status);

  std::shared_ptr<Slice> getSlice(void* curlHandle);

  const Options* options() const;

  utf8string redirectUrl() const;

  utf8string indexFilePath() const;

  void cleanup();
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
}  // namespace zoe

#endif  // !ZOE_SLICE_MANAGE_H_