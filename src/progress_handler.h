#pragma once
#include "teemo/teemo.h"
#include "slice_manager.h"

namespace teemo {
typedef struct _Options Options;

class ProgressHandler {
 public:
  ProgressHandler(Options* options, Event* stop_event, std::shared_ptr<SliceManager> slice_manager);
  virtual ~ProgressHandler();

 protected:
  void asyncTaskProcess();

 protected:
  std::shared_future<void> async_task_;
  const Options* options_;
  Event* stop_event_;
  std::shared_ptr<SliceManager> slice_manager_;
};
}  // namespace teemo