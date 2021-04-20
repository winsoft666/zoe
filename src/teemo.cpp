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
#include <assert.h>
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
                                      u8"INVALID_HASH_POLICY",
                                      u8"INVALID_SLICE_POLICY",
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
                                      u8"SLICE_DOWNLOAD_FAILED",
                                      u8"HASH_VERIFY_NOT_PASS",
                                      u8"CALCULATE_HASH_FAILED",
                                      u8"FETCH_FILE_INFO_FAILED",
                                      u8"UNSURE_DOWNLOAD_COMPLETED",
                                      u8"REDIRECT_URL_DIFFERENT"};
  return EnumStrings[enumVal];
}

class Teemo::TeemoImpl {
 public:
  TeemoImpl() {}
  ~TeemoImpl() {}

  bool isDownloading() {
    if (!entry_handler_)
      return false;
    return entry_handler_->state() != DownloadState::STOPPED;
  }
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
  assert(impl_);
  impl_->options_.verbose_functor = verbose_functor;
}

Result Teemo::setThreadNum(int32_t thread_num) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (thread_num <= 0)
    thread_num = TEEMO_DEFAULT_THREAD_NUM;
  if (thread_num > 100)
    return INVALID_THREAD_NUM;
  impl_->options_.thread_num = thread_num;
  return SUCCESSED;
}

int32_t Teemo::threadNum() const noexcept {
  assert(impl_);
  return impl_->options_.thread_num;
}

utf8string Teemo::url() const noexcept {
  assert(impl_);
  return impl_->options_.url;
}

utf8string Teemo::targetFilePath() const noexcept {
  assert(impl_);
  return impl_->options_.target_file_path;
}

int64_t Teemo::originFileSize() const noexcept {
  assert(impl_);
  int64_t ret = -1;
  if (impl_ && impl_->entry_handler_)
    ret = impl_->entry_handler_->originFileSize();
  return ret;
}

DownloadState Teemo::state() const noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_)
    return impl_->entry_handler_->state();
  return DownloadState::STOPPED;
}

Result Teemo::setNetworkConnectionTimeout(int32_t milliseconds) noexcept {
  assert(impl_);
  if (milliseconds <= 0)
    milliseconds = TEEMO_DEFAULT_NETWORK_CONN_TIMEOUT_MS;
  impl_->options_.network_conn_timeout = milliseconds;
  return SUCCESSED;
}

int32_t Teemo::networkConnectionTimeout() const noexcept {
  assert(impl_);
  return impl_->options_.network_conn_timeout;
}

Result Teemo::setFetchFileInfoRetryTimes(int32_t retry_times) noexcept {
  assert(impl_);
  if (retry_times <= 0)
    retry_times = TEEMO_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES;
  impl_->options_.fetch_file_info_retry = retry_times;
  return SUCCESSED;
}

int32_t Teemo::fetchFileInfoRetryTimes() const noexcept {
  assert(impl_);
  return impl_->options_.fetch_file_info_retry;
}

Result Teemo::setTmpFileExpiredTime(int32_t seconds) noexcept {
  assert(impl_);
  impl_->options_.tmp_file_expired_time = seconds;

  return SUCCESSED;
}

int32_t Teemo::tmpFileExpiredTime() const noexcept {
  assert(impl_);
  return impl_->options_.tmp_file_expired_time;
}

Result Teemo::setMaxDownloadSpeed(int32_t byte_per_seconds) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (byte_per_seconds <= 0)
    byte_per_seconds = -1;
  impl_->options_.max_speed = byte_per_seconds;

  return SUCCESSED;
}

int32_t Teemo::maxDownloadSpeed() const noexcept {
  assert(impl_);
  return impl_->options_.max_speed;
}

Result Teemo::setDiskCacheSize(int32_t cache_size) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (cache_size < 0)
    cache_size = 0;
  impl_->options_.disk_cache_size = cache_size;
  return SUCCESSED;
}

int32_t Teemo::diskCacheSize() const noexcept {
  assert(impl_);
  return impl_->options_.disk_cache_size;
}

Result Teemo::setStopEvent(Event* stop_event) noexcept {
  assert(impl_);
  impl_->options_.user_stop_event = stop_event;
  return SUCCESSED;
}

Event* Teemo::stopEvent() noexcept {
  assert(impl_);
  return impl_->options_.user_stop_event;
}

Result Teemo::setRedirectedUrlCheckEnabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.redirected_url_check_enabled = enabled;
  return SUCCESSED;
}

bool Teemo::redirectedUrlCheckEnabled() const noexcept {
  assert(impl_);
  return impl_->options_.redirected_url_check_enabled;
}

Result Teemo::setContentMd5Enabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.content_md5_enabled = enabled;
  return SUCCESSED;
}

bool Teemo::contentMd5Enabled() const noexcept {
  assert(impl_);
  return impl_->options_.content_md5_enabled;
}

Result Teemo::setSlicePolicy(SlicePolicy policy,
                             int64_t policy_value) noexcept {
  assert(impl_);
  if (policy == FixedSize) {
    if (policy_value <= 0)
      policy_value = TEEMO_DEFAULT_FIXED_SLICE_SIZE_BYTE;
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = policy_value;
    return SUCCESSED;
  }
  else if (policy == FixedNum) {
    if (policy_value <= 0)
      policy_value = TEEMO_DEFAULT_FIXED_SLICE_NUM;
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = policy_value;
    return SUCCESSED;
  }
  else if (policy == Auto) {
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = 0L;
    return SUCCESSED;
  }
  assert(false);
  return INVALID_SLICE_POLICY;
}

void Teemo::slicePolicy(SlicePolicy& policy, int64_t& policy_value) const
    noexcept {
  assert(impl_);
  policy = impl_->options_.slice_policy;
  policy_value = impl_->options_.slice_policy_value;
}

Result Teemo::setHashVerifyPolicy(HashVerifyPolicy policy,
                                  HashType hash_type,
                                  const utf8string& hash_value) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;

  impl_->options_.hash_verify_policy = policy;
  impl_->options_.hash_type = hash_type;
  impl_->options_.hash_value = hash_value;

  return SUCCESSED;
}

void Teemo::hashVerifyPolicy(HashVerifyPolicy& policy,
                             HashType& hash_type,
                             utf8string& hash_value) const noexcept {
  assert(impl_);
  policy = impl_->options_.hash_verify_policy;
  hash_type = impl_->options_.hash_type;
  hash_value = impl_->options_.hash_value;
}

Result Teemo::setHttpHeaders(const HttpHeaders& headers) noexcept {
  assert(impl_);
  impl_->options_.http_headers = headers;

  return SUCCESSED;
}

HttpHeaders Teemo::httpHeaders() const noexcept {
  assert(impl_);
  return impl_->options_.http_headers;
}

std::shared_future<Result> Teemo::start(
    const utf8string& url,
    const utf8string& target_file_path,
    ResultFunctor result_functor,
    ProgressFunctor progress_functor,
    RealtimeSpeedFunctor realtime_speed_functor) noexcept {
  assert(impl_);
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

  if (impl_->entry_handler_)
    impl_->entry_handler_.reset();

  impl_->entry_handler_ = std::make_shared<EntryHandler>();

  return impl_->entry_handler_->start(&impl_->options_);
}

void Teemo::pause() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->pause();
  }
}

void Teemo::resume() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->resume();
  }
}

void Teemo::stop() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->stop();
  }
}

class Event::EventImpl {
 public:
  EventImpl(bool setted) : setted_(setted) {}
  void set() noexcept {
    std::unique_lock<std::mutex> ul(setted_mutex_);
    setted_ = true;
  }

  void unset() noexcept {
    std::unique_lock<std::mutex> ul(setted_mutex_);
    setted_ = false;
  }

  bool isSetted() noexcept {
    std::unique_lock<std::mutex> ul(setted_mutex_);
    return setted_;
  }

  bool wait(int32_t millseconds) noexcept {
    std::unique_lock<std::mutex> ul(setted_mutex_);
    setted_cond_var_.wait_for(ul, std::chrono::milliseconds(millseconds),
                              [this] { return setted_; });
    return setted_;
  }

 protected:
  bool setted_;
  std::mutex setted_mutex_;
  std::condition_variable setted_cond_var_;
};

Event::Event(bool setted) : impl_(new EventImpl(setted)) {}

Event::~Event() {
  assert(impl_);
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

void Event::set() noexcept {
  assert(impl_);
  impl_->set();
}

void Event::unset() noexcept {
  assert(impl_);
  impl_->unset();
}

bool Event::isSetted() noexcept {
  assert(impl_);
  return impl_->isSetted();
}

bool Event::wait(int32_t millseconds) noexcept {
  assert(impl_);
  return impl_->wait(millseconds);
}
}  // namespace teemo
