#include "progress_handler.h"
#include "options.h"

namespace teemo {
ProgressHandler::ProgressHandler(Options* options,
                                 Event* stop_event,
                                 std::shared_ptr<SliceManager> slice_manager)
    : options_(options), stop_event_(stop_event), slice_manager_(slice_manager) {
  if (options_ && stop_event_ && slice_manager_) {
    async_task_ = std::async(std::launch::async, std::bind(&ProgressHandler::asyncTaskProcess, this));
  }
}

ProgressHandler::~ProgressHandler() {
  if (async_task_.valid())
    async_task_.get();
}

void ProgressHandler::asyncTaskProcess() {
  while (stop_event_ && !stop_event_->wait(500)) {
    if (options_ && options_->progress_functor && slice_manager_) {
      options_->progress_functor(slice_manager_->originFileSize(),
                                 slice_manager_->totalDownloaded());
    }
  }
}

}  // namespace teemo