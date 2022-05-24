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

#include "progress_handler.h"
#include <functional>
#include "options.h"

namespace TEEMO_NAMESPACE {
ProgressHandler::ProgressHandler(Options* options,
                                 std::shared_ptr<SliceManager> slice_manager)
    : options_(options), slice_manager_(slice_manager) {
  if (options_ && slice_manager_) {
    async_task_ =
        std::async(std::launch::async,
                   std::bind(&ProgressHandler::asyncTaskProcess, this));
  }
}

ProgressHandler::~ProgressHandler() {
  if (async_task_.valid())
    async_task_.get();
}

void ProgressHandler::asyncTaskProcess() {
  while (true) {
    if (options_->internal_stop_event.wait(500))
      break;
    if (options_->user_stop_event && options_->user_stop_event->isSetted())
      break;
    if (options_ && options_->progress_functor && slice_manager_) {
      options_->progress_functor(slice_manager_->originFileSize(),
                                 slice_manager_->totalDownloaded());
    }
  }
}

}  // namespace teemo