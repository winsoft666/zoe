#pragma once

#include <memory>
#include "slice_manager.h"
#include "progress_handler.h"
#include "speed_handler.h"
#include "options.h"

namespace teemo {
typedef struct _Options Options;
class EntryHandler {
 public:
  EntryHandler();
  virtual ~EntryHandler();

  std::shared_future<Result> start(Options* options);
  void stop();

  bool isDownloading();
 protected:
  Result asyncTaskProcess();
  Result _asyncTaskProcess();
  bool fetchFileInfo(int64_t& file_size) const;
  void outputVerbose(const utf8string& info);
  void calculateSliceInfo(int32_t concurrency_num, int32_t* disk_cache_per_slice, int32_t* max_speed_per_slice);

 protected:
  std::shared_future<Result> async_task_;
  Options* options_;
  std::shared_ptr<SliceManager> slice_manager_;
  std::shared_ptr<ProgressHandler> progress_handler_;
  std::shared_ptr<SpeedHandler> speed_handler_;

  void* multi_;

  Event stop_event_;
};
}  // namespace teemo