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

#include "teemo/teemo.h"
#include "file_util.h"
#include "curl_utils.h"
#include "slice_manager.h"
#include "options.h"
#include "entry_handler.h"

namespace teemo {
const char* GetResultString(int enumVal) {
  static const char* EnumStrings[] = {u8"SUCCESSED",
                                      u8"UNKNOWN_ERROR",
                                      u8"INVALID_URL",
                                      u8"INVALID_INDEX_FORMAT",
                                      u8"INVALID_TARGET_FILE_PATH",
                                      u8"INVALID_THREAD_NUM",
                                      u8"INVALID_NETWORK_CONN_TIMEOUT",
                                      u8"INVALID_NETWORK_READ_TIMEOUT",
                                      u8"INVALID_FETCH_FILE_INFO_RETRY_TIMES",
                                      u8"ALREADY_DOWNLOADING",
                                      u8"CANCELED",
                                      u8"RENAME_TMP_FILE_FAILED",
                                      u8"OPEN_INDEX_FILE_FAILED",
                                      u8"TMP_FILE_EXPIRED",
                                      u8"INIT_CURL_FAILED",
                                      u8"INIT_CURL_MULTI_FAILED",
                                      u8"SET_CURL_OPTION_FAILED",
                                      u8"ADD_CURL_HANDLE_FAILED",
                                      u8"CREATE_TARGET_FILE_FAILED",
                                      u8"CREATE_TMP_FILE_FAILED",
                                      u8"OPEN_TMP_FILE_FAILED",
                                      u8"URL_DIFFERENT",
                                      u8"TMP_FILE_SIZE_ERROR",
                                      u8"TMP_FILE_CANNOT_RW",
                                      u8"FLUSH_TMP_FILE_FAILED",
                                      u8"UPDATE_INDEX_FILE_FAILED",
                                      u8"SLICE_DOWNLOAD_FAILED"};
  return EnumStrings[enumVal];
}

class Teemo::TeemoImpl {
 public:
  TeemoImpl() {}
  ~TeemoImpl() {}

  bool isDownloading() { return (entry_handler_ && entry_handler_->isDownloading()); }

 public:
  Options options_;
  std::shared_ptr<EntryHandler> entry_handler_;
};

Teemo::Teemo() {
  impl_ = new TeemoImpl();
}

Teemo::~Teemo() {
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

void Teemo::GlobalInit() {
  static bool has_init = false;
  if (!has_init) {
    has_init = true;
    GlobalCurlInit();
  }
}

void Teemo::GlobalUnInit() {
  GlobalCurlUnInit();
}

void Teemo::setVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept {
  impl_->options_.verbose_functor = verbose_functor;
}

Result Teemo::setThreadNum(int32_t thread_num) noexcept {
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (thread_num == 0 || thread_num > 100)
    return INVALID_THREAD_NUM;

  impl_->options_.thread_num = thread_num;
  return SUCCESSED;
}

int32_t Teemo::threadNum() const noexcept {
  return impl_->options_.thread_num;
}

utf8string Teemo::url() const noexcept {
  return impl_->options_.url;
}

utf8string Teemo::targetFilePath() const noexcept {
  return impl_->options_.target_file_path;
}

Result Teemo::setNetworkConnectionTimeout(int32_t milliseconds) noexcept {
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  impl_->options_.network_conn_timeout = milliseconds;
  return SUCCESSED;
}

int32_t Teemo::networkConnectionTimeout() const noexcept {
  return impl_->options_.network_conn_timeout;
}

Result Teemo::setNetworkReadTimeout(int32_t milliseconds) noexcept {
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  impl_->options_.network_read_timeout = milliseconds;
  return SUCCESSED;
}

int32_t Teemo::networkReadTimeout() const noexcept {
  return impl_->options_.network_read_timeout;
}

Result Teemo::setFetchFileInfoRetryTimes(int32_t retry_times) noexcept {
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;

  return SUCCESSED;
}

int32_t Teemo::fetchFileInfoRetryTimes() const noexcept {
  return impl_->options_.fetch_file_info_retry;
}

void Teemo::setTmpFileExpiredTime(int32_t seconds) noexcept {
  impl_->options_.tmp_file_expired_time = seconds;
}

int32_t Teemo::tmpFileExpiredTime() const noexcept {
  return impl_->options_.tmp_file_expired_time;
}

void Teemo::setMaxDownloadSpeed(int32_t byte_per_seconds) noexcept {
  impl_->options_.max_speed = byte_per_seconds;
}

int32_t Teemo::maxDownloadSpeed() const noexcept {
  return impl_->options_.max_speed;
}

Result Teemo::setDiskCacheSize(int32_t cache_size) noexcept {
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  impl_->options_.disk_cache_size = cache_size;
  return SUCCESSED;
}

int32_t Teemo::diskCacheSize() const noexcept {
  return impl_->options_.disk_cache_size;
}

std::shared_future<Result> Teemo::start(const utf8string& url,
                                        const utf8string& target_file_path,
                                        ResultFunctor result_functor,
                                        ProgressFunctor progress_functor,
                                        RealtimeSpeedFunctor realtime_speed_functor,
                                        bool can_update_url) noexcept {
  Result ret = SUCCESSED;

  if (impl_->isDownloading())
    ret = ALREADY_DOWNLOADING;
  else if (url.length() == 0)
    ret = INVALID_URL;
  else if (target_file_path.length() == 0)
    ret = INVALID_TARGET_FILE_PATH;

  if (ret != SUCCESSED) {
    return std::async(std::launch::async, [result_functor, ret]() {
      if (result_functor)
        result_functor(ret);
      return ret;
    });
  }

  impl_->options_.url = url;
  impl_->options_.target_file_path = target_file_path;
  impl_->options_.result_functor = result_functor;
  impl_->options_.progress_functor = progress_functor;
  impl_->options_.speed_functor = realtime_speed_functor;

  impl_->entry_handler_ = std::make_shared<EntryHandler>();

  return impl_->entry_handler_->start(&impl_->options_);
}

void Teemo::stop(bool wait) noexcept {
  impl_->entry_handler_->stop();
}

Event::Event(bool setted) : setted_(setted) {}

Event::~Event() {}

void Event::set() noexcept {
  std::unique_lock<std::mutex> ul(setted_mutex_);
  setted_ = true;
}

void Event::unSet() noexcept {
  std::unique_lock<std::mutex> ul(setted_mutex_);
  setted_ = false;
}

bool Event::isSetted() noexcept {
  std::unique_lock<std::mutex> ul(setted_mutex_);
  return setted_;
}

bool Event::wait(int32_t millseconds) noexcept {
  std::unique_lock<std::mutex> ul(setted_mutex_);
  setted_cond_var_.wait_for(ul, std::chrono::milliseconds(millseconds), [this] { return setted_; });
  return setted_;
}

}  // namespace teemo
