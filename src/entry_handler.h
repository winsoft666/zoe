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

#ifndef ZOE_ENTRY_HANDLER_H__
#define ZOE_ENTRY_HANDLER_H__
#pragma once

#include <memory>
#include "slice_manager.h"
#include "progress_handler.h"
#include "speed_handler.h"
#include "options.h"
#include "curl_utils.h"

namespace zoe {

class EntryHandler {
 public:
  typedef struct _FileInfo {
    bool acceptRanges;
    int64_t fileSize;
    utf8string contentMd5;
    utf8string redirect_url;

    void clear() {
      acceptRanges = true;
      fileSize = -1;
      contentMd5.clear();
      redirect_url.clear();
    }
    _FileInfo() {
      acceptRanges = true;
      fileSize = -1L;
    }
  } FileInfo;

  EntryHandler();
  virtual ~EntryHandler();

  std::shared_future<Result> start(Options* options);
  void pause();
  void resume();
  void stop();

  int64_t originFileSize() const;
  Options* options();

  DownloadState state() const;

  std::shared_future<Result> futureResult();
 protected:
  Result asyncTaskProcess();
  Result _asyncTaskProcess();

  bool fetchFileInfo(FileInfo& fileInfo);
  bool requestFileInfo(const utf8string& url, FileInfo& fileInfo);
  void cancelFetchFileInfo();
  void calculateSliceInfo(int32_t concurrency_num,
                          int64_t* disk_cache_per_slice,
                          int64_t* max_speed_per_slice) const;
  void updateSliceStatus();

 protected:
  std::shared_future<Result> async_task_;
  Options* options_;
  std::shared_ptr<SliceManager> slice_manager_;
  std::shared_ptr<ProgressHandler> progress_handler_;
  std::shared_ptr<SpeedHandler> speed_handler_;

  void* multi_;

  std::shared_ptr<ScopedCurl> fetch_file_info_curl_;

  std::atomic_bool user_stopped_;
  std::atomic_bool user_paused_;

  std::atomic<DownloadState> state_;
};
}  // namespace zoe
#endif  // !ZOE_ENTRY_HANDLER_H__