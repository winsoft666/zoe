#include "speed_handler.h"
#include "options.h"

namespace teemo {

SpeedHandler::SpeedHandler(int64_t already_download,
                           Options* options,
                           Event* stop_event,
                           std::shared_ptr<SliceManager> slice_manager)
    : already_download_(already_download)
    , options_(options)
    , stop_event_(stop_event)
    , slice_manager_(slice_manager) {
  if (options_ && stop_event_ && slice_manager_) {
    async_task_ = std::async(std::launch::async, std::bind(&SpeedHandler::asyncTaskProcess, this));
  }
}

SpeedHandler::~SpeedHandler() {
  if (async_task_.valid())
    async_task_.get();
}

void SpeedHandler::asyncTaskProcess() {
  while (stop_event_ && !stop_event_->wait(1000)) {
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