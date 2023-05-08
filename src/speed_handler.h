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

#ifndef LIBGET_SPEED_HANDLER_H_
#define LIBGET_SPEED_HANDLER_H_
#pragma once

#include "libGet/libGet.h"
#include "slice_manager.h"

namespace LIBGET_NAMESPACE {
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
  int64_t last_download_;
  const Options* options_;
  std::shared_ptr<SliceManager> slice_manager_;
};
}  // namespace libGet
#endif  // !LIBGET_SPEED_HANDLER_H_