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

#include "speed_handler.h"
#include <functional>
#include "options.h"

namespace teemo {
SpeedHandler::SpeedHandler(int64_t already_download,
                           Options* options,
                           std::shared_ptr<SliceManager> slice_manager)
    : already_download_(already_download), options_(options), slice_manager_(slice_manager) {
  if (options_ && slice_manager_) {
    async_task_ = std::async(std::launch::async, std::bind(&SpeedHandler::asyncTaskProcess, this));
  }
}

SpeedHandler::~SpeedHandler() {
  if (async_task_.valid())
    async_task_.get();
}

void SpeedHandler::asyncTaskProcess() {
  while ((!options_->internal_stop_event.wait(1000)) ||
         (options_->user_stop_event && options_->user_stop_event->isSetted())) {
    if (options_ && slice_manager_) {
      int64_t now = slice_manager_->totalDownloaded();

      static int64_t last = already_download_;
      if (now >= last) {
        int64_t downloaded = now - last;
        last = now;
        options_->speed_functor(downloaded);
      }
    }
  }
}

}  // namespace teemo