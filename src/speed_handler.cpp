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

#include "speed_handler.h"
#include <functional>
#include "options.h"

namespace zoe {
SpeedHandler::SpeedHandler(int64_t already_download,
                           Options* options,
                           std::shared_ptr<SliceManager> slice_manager)
    : already_download_(already_download)
    , last_download_(0L)
    , options_(options)
    , slice_manager_(slice_manager) {
  if (options_ && slice_manager_) {
    async_task_ = std::async(std::launch::async,
                             std::bind(&SpeedHandler::asyncTaskProcess, this));
  }
}

SpeedHandler::~SpeedHandler() {
  if (async_task_.valid())
    async_task_.get();
}

void SpeedHandler::asyncTaskProcess() {
  last_download_ = already_download_;
  while (true) {
    if (options_->internal_stop_event.wait(1000))
      break;
    if (options_->user_stop_event && options_->user_stop_event->isSetted())
      break;
    if (options_ && slice_manager_) {
      const int64_t now = slice_manager_->totalDownloaded();

      if (now >= last_download_) {
        const int64_t downloaded = now - last_download_;
        last_download_ = now;
        options_->speed_functor(downloaded);
      }
    }
  }
}

}  // namespace zoe