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

#ifndef TEEMO_TIME_METER_H_
#define TEEMO_TIME_METER_H_
#pragma once

#include <stdint.h>
#include <time.h>
#include <ctime>
#include <limits>

namespace teemo {
class TimeMeter {
 public:
  TimeMeter() { lStartTime_ = std::clock(); }

  void Restart() { lStartTime_ = std::clock(); }

  // ms
  long Elapsed() const { return std::clock() - lStartTime_; }

  long ElapsedMax() const {
    return (std::numeric_limits<std::clock_t>::max)() - lStartTime_;
  }

  long ElapsedMin() const { return 1L; }

 private:
  std::clock_t lStartTime_;
};
}  // namespace teemo

#endif
