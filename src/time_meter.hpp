/*******************************************************************************
* Copyright (C) 2019 - 2023, winsoft666, <winsoft666@outlook.com>.
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
