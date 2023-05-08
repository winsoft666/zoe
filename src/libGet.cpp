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

#include "libGet/libGet.h"
#include <assert.h>
#include "file_util.h"
#include "curl_utils.h"
#include "slice_manager.h"
#include "options.h"
#include "entry_handler.h"
#include "string_helper.hpp"

namespace LIBGET_NAMESPACE {
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
                                      u8"REDIRECT_URL_DIFFERENT",
                                      u8"NOT_CLEARLY_RESULT"};
  return EnumStrings[enumVal];
}

class LIBGET::TeemoImpl {
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

LIBGET::LIBGET() {
  impl_ = new TeemoImpl();
}

LIBGET::~LIBGET() {
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

void LIBGET::GlobalInit() {
  static bool has_init = false;
  if (!has_init) {
    has_init = true;
    GlobalCurlInit();
  }
}

void LIBGET::GlobalUnInit() {
  GlobalCurlUnInit();
}

void LIBGET::setVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept {
  assert(impl_);
  impl_->options_.verbose_functor = verbose_functor;
}

Result LIBGET::setThreadNum(int32_t thread_num) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (thread_num <= 0)
    thread_num = LIBGET_DEFAULT_THREAD_NUM;
  if (thread_num > 100)
    return INVALID_THREAD_NUM;
  impl_->options_.thread_num = thread_num;
  return SUCCESSED;
}

int32_t LIBGET::threadNum() const noexcept {
  assert(impl_);
  return impl_->options_.thread_num;
}

utf8string LIBGET::url() const noexcept {
  assert(impl_);
  return impl_->options_.url;
}

utf8string LIBGET::targetFilePath() const noexcept {
  assert(impl_);
  return impl_->options_.target_file_path;
}

int64_t LIBGET::originFileSize() const noexcept {
  assert(impl_);
  int64_t ret = -1;
  if (impl_ && impl_->entry_handler_)
    ret = impl_->entry_handler_->originFileSize();
  return ret;
}

DownloadState LIBGET::state() const noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_)
    return impl_->entry_handler_->state();
  return DownloadState::STOPPED;
}

std::shared_future<LIBGET_NAMESPACE::Result> LIBGET::futureResult() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_)
    return impl_->entry_handler_->futureResult();
  return std::shared_future<LIBGET_NAMESPACE::Result>();
}

Result LIBGET::setNetworkConnectionTimeout(int32_t milliseconds) noexcept {
  assert(impl_);
  if (milliseconds <= 0)
    milliseconds = LIBGET_DEFAULT_NETWORK_CONN_TIMEOUT_MS;
  impl_->options_.network_conn_timeout = milliseconds;
  return SUCCESSED;
}

int32_t LIBGET::networkConnectionTimeout() const noexcept {
  assert(impl_);
  return impl_->options_.network_conn_timeout;
}

Result LIBGET::setFetchFileInfoRetryTimes(int32_t retry_times) noexcept {
  assert(impl_);
  if (retry_times <= 0)
    retry_times = LIBGET_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES;
  impl_->options_.fetch_file_info_retry = retry_times;
  return SUCCESSED;
}

int32_t LIBGET::fetchFileInfoRetryTimes() const noexcept {
  assert(impl_);
  return impl_->options_.fetch_file_info_retry;
}

Result LIBGET::setFetchFileInfoHeadMethod(bool use_head) noexcept {
  assert(impl_);
  impl_->options_.use_head_method_fetch_file_info = use_head;
  return SUCCESSED;
}

bool LIBGET::fetchFileInfoHeadMethod() const noexcept {
  assert(impl_);
  return impl_->options_.use_head_method_fetch_file_info;
}

Result LIBGET::setTmpFileExpiredTime(int32_t seconds) noexcept {
  assert(impl_);
  impl_->options_.tmp_file_expired_time = seconds;

  return SUCCESSED;
}

int32_t LIBGET::tmpFileExpiredTime() const noexcept {
  assert(impl_);
  return impl_->options_.tmp_file_expired_time;
}

Result LIBGET::setMaxDownloadSpeed(int32_t byte_per_seconds) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (byte_per_seconds <= 0)
    byte_per_seconds = -1;
  impl_->options_.max_speed = byte_per_seconds;

  return SUCCESSED;
}

int32_t LIBGET::maxDownloadSpeed() const noexcept {
  assert(impl_);
  return impl_->options_.max_speed;
}

Result LIBGET::setMinDownloadSpeed(int32_t byte_per_seconds,
                                  int32_t duration) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (byte_per_seconds <= 0)
    byte_per_seconds = -1;
  impl_->options_.min_speed = byte_per_seconds;
  impl_->options_.min_speed_duration = duration;

  return SUCCESSED;
}

int32_t LIBGET::minDownloadSpeed() const noexcept {
  assert(impl_);
  return impl_->options_.min_speed;
}

int32_t LIBGET::minDownloadSpeedDuration() const noexcept {
  assert(impl_);
  return impl_->options_.min_speed_duration;
}

Result LIBGET::setDiskCacheSize(int32_t cache_size) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ALREADY_DOWNLOADING;
  if (cache_size < 0)
    cache_size = 0;
  impl_->options_.disk_cache_size = cache_size;
  return SUCCESSED;
}

int32_t LIBGET::diskCacheSize() const noexcept {
  assert(impl_);
  return impl_->options_.disk_cache_size;
}

Result LIBGET::setStopEvent(Event* stop_event) noexcept {
  assert(impl_);
  impl_->options_.user_stop_event = stop_event;
  return SUCCESSED;
}

Event* LIBGET::stopEvent() noexcept {
  assert(impl_);
  return impl_->options_.user_stop_event;
}

Result LIBGET::setRedirectedUrlCheckEnabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.redirected_url_check_enabled = enabled;
  return SUCCESSED;
}

bool LIBGET::redirectedUrlCheckEnabled() const noexcept {
  assert(impl_);
  return impl_->options_.redirected_url_check_enabled;
}

Result LIBGET::setContentMd5Enabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.content_md5_enabled = enabled;
  return SUCCESSED;
}

bool LIBGET::contentMd5Enabled() const noexcept {
  assert(impl_);
  return impl_->options_.content_md5_enabled;
}

Result LIBGET::setSlicePolicy(SlicePolicy policy,
                             int64_t policy_value) noexcept {
  assert(impl_);
  if (policy == FixedSize) {
    if (policy_value <= 0)
      policy_value = LIBGET_DEFAULT_FIXED_SLICE_SIZE_BYTE;
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = policy_value;
    return SUCCESSED;
  }
  else if (policy == FixedNum) {
    if (policy_value <= 0)
      policy_value = LIBGET_DEFAULT_FIXED_SLICE_NUM;
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

void LIBGET::slicePolicy(SlicePolicy& policy, int64_t& policy_value) const
    noexcept {
  assert(impl_);
  policy = impl_->options_.slice_policy;
  policy_value = impl_->options_.slice_policy_value;
}

Result LIBGET::setHashVerifyPolicy(HashVerifyPolicy policy,
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

void LIBGET::hashVerifyPolicy(HashVerifyPolicy& policy,
                             HashType& hash_type,
                             utf8string& hash_value) const noexcept {
  assert(impl_);
  policy = impl_->options_.hash_verify_policy;
  hash_type = impl_->options_.hash_type;
  hash_value = impl_->options_.hash_value;
}

Result LIBGET::setHttpHeaders(const HttpHeaders& headers) noexcept {
  assert(impl_);
  impl_->options_.http_headers = headers;

  return SUCCESSED;
}

HttpHeaders LIBGET::httpHeaders() const noexcept {
  assert(impl_);
  return impl_->options_.http_headers;
}

Result LIBGET::setProxy(const utf8string& proxy) noexcept {
  assert(impl_);
  impl_->options_.proxy = proxy;

  return SUCCESSED;
}

utf8string LIBGET::proxy() const noexcept {
  assert(impl_);
  return impl_->options_.proxy;
}

Result LIBGET::setUncompletedSliceSavePolicy(UncompletedSliceSavePolicy policy) noexcept {
  assert(impl_);
  impl_->options_.uncompleted_slice_save_policy = policy;

  return SUCCESSED;
}

UncompletedSliceSavePolicy LIBGET::uncompletedSliceSavePolicy() const noexcept {
  assert(impl_);
  return impl_->options_.uncompleted_slice_save_policy;
}

std::shared_future<Result> LIBGET::start(
    const utf8string& url,
    const utf8string& target_file_path,
    ResultFunctor result_functor,
    ProgressFunctor progress_functor,
    RealtimeSpeedFunctor realtime_speed_functor) noexcept {
  assert(impl_);
  Result ret = SUCCESSED;

  utf8string target_path_formatted;

  if (impl_->isDownloading()) {
    ret = ALREADY_DOWNLOADING;
  }
  else if (url.length() == 0) {
    ret = INVALID_URL;
  }
  else {
    if (!FileUtil::PathFormatting(target_file_path, target_path_formatted))
      ret = INVALID_TARGET_FILE_PATH;
  }

  if (ret != SUCCESSED) {
    return std::async(std::launch::async, [result_functor, ret]() {
      if (result_functor)
        result_functor(ret);
      return ret;
    });
  }

  impl_->options_.url = StringHelper::Trim(url);
  impl_->options_.target_file_path = target_path_formatted;
  impl_->options_.result_functor = result_functor;
  impl_->options_.progress_functor = progress_functor;
  impl_->options_.speed_functor = realtime_speed_functor;

  if (impl_->entry_handler_)
    impl_->entry_handler_.reset();

  impl_->entry_handler_ = std::make_shared<EntryHandler>();

  return impl_->entry_handler_->start(&impl_->options_);
}

void LIBGET::pause() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->pause();
  }
}

void LIBGET::resume() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->resume();
  }
}

void LIBGET::stop() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->stop();
  }
}

class Event::EventImpl {
 public:
  EventImpl(bool setted)
      : setted_(setted) {}
  void set() noexcept {
    std::unique_lock<std::mutex> ul(setted_mutex_);
    setted_ = true;
    setted_cond_var_.notify_all();
  }

  void unset() noexcept {
    std::unique_lock<std::mutex> ul(setted_mutex_);
    setted_ = false;
    setted_cond_var_.notify_all();
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

Event::Event(bool setted)
    : impl_(new EventImpl(setted)) {}

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
}  // namespace libGet
