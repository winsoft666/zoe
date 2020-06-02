#pragma once

#include "teemo/teemo.h"
#include "slice_manager.h"

namespace teemo {
typedef struct _Options Options;

class SpeedHandler {
 public:
  SpeedHandler(int64_t already_download,
               Options* options,
               Event* stop_event,
               std::shared_ptr<SliceManager> slice_manager);
  virtual ~SpeedHandler();

 protected:
  void asyncTaskProcess();

 protected:
  std::shared_future<void> async_task_;
  const int64_t already_download_;
  const Options* options_;
  Event* stop_event_;
  std::shared_ptr<SliceManager> slice_manager_;
};
}  // namespace teemo