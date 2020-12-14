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
#ifndef TEEMO_ENTRY_HANDLER_H__
#define TEEMO_ENTRY_HANDLER_H__
#pragma once

#include <memory>
#include "slice_manager.h"
#include "progress_handler.h"
#include "speed_handler.h"
#include "options.h"

namespace teemo {
typedef struct _Options Options;
class EntryHandler {
 public:
  EntryHandler();
  virtual ~EntryHandler();

  std::shared_future<Result> start(Options* options);
  void stop();

  bool isDownloading();
  int64_t originFileSize() const;
 protected:
  Result asyncTaskProcess();
  Result _asyncTaskProcess();
  bool fetchFileInfo(int64_t& file_size) const;
  void outputVerbose(const utf8string& info) const;
  void calculateSliceInfo(int32_t concurrency_num, int32_t* disk_cache_per_slice, int32_t* max_speed_per_slice);

 protected:
  std::shared_future<Result> async_task_;
  Options* options_;
  std::shared_ptr<SliceManager> slice_manager_;
  std::shared_ptr<ProgressHandler> progress_handler_;
  std::shared_ptr<SpeedHandler> speed_handler_;

  void* multi_;

  std::atomic_bool user_stop_;
};
}  // namespace teemo
#endif // !TEEMO_ENTRY_HANDLER_H__