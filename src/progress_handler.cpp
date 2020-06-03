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

#include "progress_handler.h"
#include <functional>
#include "options.h"

namespace teemo {
ProgressHandler::ProgressHandler(Options* options, std::shared_ptr<SliceManager> slice_manager)
    : options_(options), slice_manager_(slice_manager) {
  if (options_ && slice_manager_) {
    async_task_ =
        std::async(std::launch::async, std::bind(&ProgressHandler::asyncTaskProcess, this));
  }
}

ProgressHandler::~ProgressHandler() {
  if (async_task_.valid())
    async_task_.get();
}

void ProgressHandler::asyncTaskProcess() {
  while (!options_->internal_stop_event.wait(500) ||
         (options_->user_stop_event && !options_->user_stop_event->isSetted())) {
    if (options_ && options_->progress_functor && slice_manager_) {
      options_->progress_functor(slice_manager_->originFileSize(),
                                 slice_manager_->totalDownloaded());
    }
  }
}

}  // namespace teemo