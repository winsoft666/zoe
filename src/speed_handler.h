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

#ifndef TEEMO_SPEED_HANDLER_H_
#define TEEMO_SPEED_HANDLER_H_
#pragma once

#include "teemo/teemo.h"
#include "slice_manager.h"

namespace teemo {
typedef struct _Options Options;

class SpeedHandler {
 public:
  SpeedHandler(int64_t already_download,
               Options* options,
               std::shared_ptr<SliceManager> slice_manager);
  virtual ~SpeedHandler();

 protected:
  void asyncTaskProcess();

 protected:
  std::shared_future<void> async_task_;
  const int64_t already_download_;
  const Options* options_;
  std::shared_ptr<SliceManager> slice_manager_;
};
}  // namespace teemo
#endif  // !TEEMO_SPEED_HANDLER_H_