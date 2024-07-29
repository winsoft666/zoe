/*******************************************************************************
*    Copyright (C) <2019-2024>, winsoft666, <winsoft666@outlook.com>.
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

#include "zoe/zoe.h"
#include <assert.h>
#include "file_util.h"
#include "curl_utils.h"
#include "slice_manager.h"
#include "options.h"
#include "entry_handler.h"
#include "string_helper.hpp"
#include "string_encode.h"

namespace zoe {
const char* Zoe::GetResultString(ZoeResult enumVal) {
  static const char* EnumStrings[] = {"SUCCESSED",
                                      "UNKNOWN_ERROR",
                                      "INVALID_URL",
                                      "INVALID_INDEX_FORMAT",
                                      "INVALID_TARGET_FILE_PATH",
                                      "INVALID_THREAD_NUM",
                                      "INVALID_HASH_POLICY",
                                      "INVALID_SLICE_POLICY",
                                      "INVALID_NETWORK_CONN_TIMEOUT",
                                      "INVALID_NETWORK_READ_TIMEOUT",
                                      "INVALID_FETCH_FILE_INFO_RETRY_TIMES",
                                      "ALREADY_DOWNLOADING",
                                      "CANCELED",
                                      "RENAME_TMP_FILE_FAILED",
                                      "OPEN_INDEX_FILE_FAILED",
                                      "TMP_FILE_EXPIRED",
                                      "INIT_CURL_FAILED",
                                      "INIT_CURL_MULTI_FAILED",
                                      "SET_CURL_OPTION_FAILED",
                                      "ADD_CURL_HANDLE_FAILED",
                                      "CREATE_TARGET_FILE_FAILED",
                                      "CREATE_TMP_FILE_FAILED",
                                      "OPEN_TMP_FILE_FAILED",
                                      "URL_DIFFERENT",
                                      "TMP_FILE_SIZE_ERROR",
                                      "TMP_FILE_CANNOT_RW",
                                      "FLUSH_TMP_FILE_FAILED",
                                      "UPDATE_INDEX_FILE_FAILED",
                                      "SLICE_DOWNLOAD_FAILED",
                                      "HASH_VERIFY_NOT_PASS",
                                      "CALCULATE_HASH_FAILED",
                                      "FETCH_FILE_INFO_FAILED",
                                      "REDIRECT_URL_DIFFERENT",
                                      "NOT_CLEARLY_RESULT"};
  return EnumStrings[(int)enumVal];
}

class Zoe::ZoeImpl {
 public:
  ZoeImpl() {}
  ~ZoeImpl() {}

  bool isDownloading() {
    if (!entry_handler_)
      return false;
    return entry_handler_->state() != DownloadState::Stopped;
  }

 public:
  Options options_;
  std::shared_ptr<EntryHandler> entry_handler_;
};

Zoe::Zoe() noexcept {
  impl_ = new ZoeImpl();
}

Zoe::~Zoe() noexcept {
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

void Zoe::GlobalInit() {
  static bool has_init = false;
  if (!has_init) {
    has_init = true;
    GlobalCurlInit();
  }
}

void Zoe::GlobalUnInit() {
  GlobalCurlUnInit();
}

void Zoe::setVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept {
  assert(impl_);
  impl_->options_.verbose_functor = verbose_functor;
}

ZoeResult Zoe::setThreadNum(int32_t thread_num) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ZoeResult::ALREADY_DOWNLOADING;
  if (thread_num <= 0)
    thread_num = ZOE_DEFAULT_THREAD_NUM;
  if (thread_num > 100)
    return ZoeResult::INVALID_THREAD_NUM;
  impl_->options_.thread_num = thread_num;
  return ZoeResult::SUCCESSED;
}

int32_t Zoe::threadNum() const noexcept {
  assert(impl_);
  return impl_->options_.thread_num;
}

utf8string Zoe::url() const noexcept {
  assert(impl_);
  return impl_->options_.url;
}

utf8string Zoe::targetFilePath() const noexcept {
  assert(impl_);
  return impl_->options_.target_file_path;
}

int64_t Zoe::originFileSize() const noexcept {
  assert(impl_);
  int64_t ret = -1;
  if (impl_ && impl_->entry_handler_)
    ret = impl_->entry_handler_->originFileSize();
  return ret;
}

DownloadState Zoe::state() const noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_)
    return impl_->entry_handler_->state();
  return DownloadState::Stopped;
}

std::shared_future<ZoeResult> Zoe::futureResult() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_)
    return impl_->entry_handler_->futureResult();
  return std::shared_future<ZoeResult>();
}

ZoeResult Zoe::setNetworkConnectionTimeout(int32_t milliseconds) noexcept {
  assert(impl_);
  if (milliseconds <= 0)
    milliseconds = ZOE_DEFAULT_NETWORK_CONN_TIMEOUT_MS;
  impl_->options_.network_conn_timeout = milliseconds;
  return ZoeResult::SUCCESSED;
}

int32_t Zoe::networkConnectionTimeout() const noexcept {
  assert(impl_);
  return impl_->options_.network_conn_timeout;
}

ZoeResult Zoe::setRetryTimesOfFetchFileInfo(int32_t retry_times) noexcept {
  assert(impl_);
  if (retry_times <= 0)
    retry_times = ZOE_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES;
  impl_->options_.fetch_file_info_retry = retry_times;
  return ZoeResult::SUCCESSED;
}

int32_t Zoe::retryTimesOfFetchFileInfo() const noexcept {
  assert(impl_);
  return impl_->options_.fetch_file_info_retry;
}

ZoeResult Zoe::setFetchFileInfoHeadMethodEnabled(bool use_head) noexcept {
  assert(impl_);
  impl_->options_.use_head_method_fetch_file_info = use_head;
  return ZoeResult::SUCCESSED;
}

bool Zoe::fetchFileInfoHeadMethodEnabled() const noexcept {
  assert(impl_);
  return impl_->options_.use_head_method_fetch_file_info;
}

ZoeResult Zoe::setExpiredTimeOfTmpFile(int32_t seconds) noexcept {
  assert(impl_);
  impl_->options_.tmp_file_expired_time = seconds;

  return ZoeResult::SUCCESSED;
}

int32_t Zoe::expiredTimeOfTmpFile() const noexcept {
  assert(impl_);
  return impl_->options_.tmp_file_expired_time;
}

ZoeResult Zoe::setMaxDownloadSpeed(int32_t byte_per_seconds) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ZoeResult::ALREADY_DOWNLOADING;
  if (byte_per_seconds <= 0)
    byte_per_seconds = -1;
  impl_->options_.max_speed = byte_per_seconds;

  return ZoeResult::SUCCESSED;
}

int32_t Zoe::maxDownloadSpeed() const noexcept {
  assert(impl_);
  return impl_->options_.max_speed;
}

ZoeResult Zoe::setMinDownloadSpeed(int32_t byte_per_seconds,
                                   int32_t duration) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ZoeResult::ALREADY_DOWNLOADING;
  if (byte_per_seconds <= 0)
    byte_per_seconds = -1;
  impl_->options_.min_speed = byte_per_seconds;
  impl_->options_.min_speed_duration = duration;

  return ZoeResult::SUCCESSED;
}

int32_t Zoe::minDownloadSpeed() const noexcept {
  assert(impl_);
  return impl_->options_.min_speed;
}

int32_t Zoe::minDownloadSpeedDuration() const noexcept {
  assert(impl_);
  return impl_->options_.min_speed_duration;
}

ZoeResult Zoe::setDiskCacheSize(int32_t cache_size) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ZoeResult::ALREADY_DOWNLOADING;
  if (cache_size < 0)
    cache_size = 0;
  impl_->options_.disk_cache_size = cache_size;
  return ZoeResult::SUCCESSED;
}

int32_t Zoe::diskCacheSize() const noexcept {
  assert(impl_);
  return impl_->options_.disk_cache_size;
}

ZoeResult Zoe::setStopEvent(ZoeEvent* stop_event) noexcept {
  assert(impl_);
  impl_->options_.user_stop_event = stop_event;
  return ZoeResult::SUCCESSED;
}

ZoeEvent* Zoe::stopEvent() noexcept {
  assert(impl_);
  return impl_->options_.user_stop_event;
}

ZoeResult Zoe::setRedirectedUrlCheckEnabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.redirected_url_check_enabled = enabled;
  return ZoeResult::SUCCESSED;
}

bool Zoe::redirectedUrlCheckEnabled() const noexcept {
  assert(impl_);
  return impl_->options_.redirected_url_check_enabled;
}

ZoeResult Zoe::setContentMd5Enabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.content_md5_enabled = enabled;
  return ZoeResult::SUCCESSED;
}

bool Zoe::contentMd5Enabled() const noexcept {
  assert(impl_);
  return impl_->options_.content_md5_enabled;
}

ZoeResult Zoe::setSlicePolicy(SlicePolicy policy,
                              int64_t policy_value) noexcept {
  assert(impl_);
  if (policy == SlicePolicy::FixedSize) {
    if (policy_value <= 0)
      policy_value = ZOE_DEFAULT_FIXED_SLICE_SIZE_BYTE;
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = policy_value;
    return ZoeResult::SUCCESSED;
  }
  else if (policy == SlicePolicy::FixedNum) {
    if (policy_value <= 0)
      policy_value = ZOE_DEFAULT_FIXED_SLICE_NUM;
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = policy_value;
    return ZoeResult::SUCCESSED;
  }
  else if (policy == SlicePolicy::Auto) {
    impl_->options_.slice_policy = policy;
    impl_->options_.slice_policy_value = 0L;
    return ZoeResult::SUCCESSED;
  }
  assert(false);
  return ZoeResult::INVALID_SLICE_POLICY;
}

void Zoe::slicePolicy(SlicePolicy& policy, int64_t& policy_value) const
    noexcept {
  assert(impl_);
  policy = impl_->options_.slice_policy;
  policy_value = impl_->options_.slice_policy_value;
}

ZoeResult Zoe::setHashVerifyPolicy(HashVerifyPolicy policy,
                                   HashType hash_type,
                                   const utf8string& hash_value) noexcept {
  assert(impl_);
  if (impl_->isDownloading())
    return ZoeResult::ALREADY_DOWNLOADING;

  impl_->options_.hash_verify_policy = policy;
  impl_->options_.hash_type = hash_type;
  impl_->options_.hash_value = hash_value;

  return ZoeResult::SUCCESSED;
}

void Zoe::hashVerifyPolicy(HashVerifyPolicy& policy,
                           HashType& hash_type,
                           utf8string& hash_value) const noexcept {
  assert(impl_);
  policy = impl_->options_.hash_verify_policy;
  hash_type = impl_->options_.hash_type;
  hash_value = impl_->options_.hash_value;
}

ZoeResult Zoe::setHttpHeaders(const HttpHeaders& headers) noexcept {
  assert(impl_);
  impl_->options_.http_headers = headers;

  return ZoeResult::SUCCESSED;
}

HttpHeaders Zoe::httpHeaders() const noexcept {
  assert(impl_);
  return impl_->options_.http_headers;
}

ZoeResult Zoe::setProxy(const utf8string& proxy) noexcept {
  assert(impl_);
  impl_->options_.proxy = proxy;

  return ZoeResult::SUCCESSED;
}

utf8string Zoe::proxy() const noexcept {
  assert(impl_);
  return impl_->options_.proxy;
}

ZoeResult Zoe::setVerifyCAEnabled(bool enabled, const utf8string& ca_path) noexcept {
  assert(impl_);
  impl_->options_.verify_peer_certificate = enabled;
  impl_->options_.ca_path = ca_path;

  return ZoeResult::SUCCESSED;
}

bool Zoe::verifyCAEnabled() const noexcept {
  assert(impl_);
  return impl_->options_.verify_peer_certificate;
}

utf8string Zoe::caPath() const noexcept {
  assert(impl_);
  return impl_->options_.ca_path;
}

ZoeResult Zoe::setVerifyHostEnabled(bool enabled) noexcept {
  assert(impl_);
  impl_->options_.verify_peer_host = enabled;

  return ZoeResult::SUCCESSED;
}

bool Zoe::verifyHostEnabled() const noexcept {
  assert(impl_);
  return impl_->options_.verify_peer_host;
}

ZoeResult Zoe::setUncompletedSliceSavePolicy(UncompletedSliceSavePolicy policy) noexcept {
  assert(impl_);
  impl_->options_.uncompleted_slice_save_policy = policy;

  return ZoeResult::SUCCESSED;
}

UncompletedSliceSavePolicy Zoe::uncompletedSliceSavePolicy() const noexcept {
  assert(impl_);
  return impl_->options_.uncompleted_slice_save_policy;
}

std::shared_future<ZoeResult> Zoe::start(
    const utf8string& url,
    const utf8string& target_file_path,
    ResultFunctor result_functor,
    ProgressFunctor progress_functor,
    RealtimeSpeedFunctor realtime_speed_functor) noexcept {
  assert(impl_);
  ZoeResult ret = ZoeResult::SUCCESSED;

  utf8string target_path_formatted;

  if (impl_->isDownloading()) {
    ret = ZoeResult::ALREADY_DOWNLOADING;
  }
  else if (url.length() == 0) {
    ret = ZoeResult::INVALID_URL;
  }
  else {
    if (!FileUtil::PathFormatting(target_file_path, target_path_formatted))
      ret = ZoeResult::INVALID_TARGET_FILE_PATH;
  }

  if (ret != ZoeResult::SUCCESSED) {
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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
std::shared_future<ZoeResult> Zoe::start(
    const std::wstring& url,
    const std::wstring& target_file_path,
    ResultFunctor result_functor,
    ProgressFunctor progress_functor,
    RealtimeSpeedFunctor realtime_speed_functor) noexcept {
  utf8string url_u8 = UnicodeToUtf8(url);
  utf8string target_file_path_u8 = UnicodeToUtf8(target_file_path);

  return start(url_u8, target_file_path_u8, result_functor, progress_functor, realtime_speed_functor);
}
#endif

void Zoe::pause() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->pause();
  }
}

void Zoe::resume() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->resume();
  }
}

void Zoe::stop() noexcept {
  assert(impl_);
  if (impl_ && impl_->entry_handler_) {
    impl_->entry_handler_->stop();
  }
}

class ZoeEvent::EventImpl {
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

ZoeEvent::ZoeEvent(bool setted)
    : impl_(new EventImpl(setted)) {}

ZoeEvent::~ZoeEvent() {
  assert(impl_);
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

void ZoeEvent::set() noexcept {
  assert(impl_);
  impl_->set();
}

void ZoeEvent::unset() noexcept {
  assert(impl_);
  impl_->unset();
}

bool ZoeEvent::isSetted() noexcept {
  assert(impl_);
  return impl_->isSetted();
}

bool ZoeEvent::wait(int32_t millseconds) noexcept {
  assert(impl_);
  return impl_->wait(millseconds);
}
}  // namespace zoe
