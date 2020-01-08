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

#include "teemo.h"
#include "file_util.h"
#include "curl_utils.h"
#include "slice_manage.h"

namespace teemo {
const char *GetResultString(int enumVal) {
  static const char *EnumStrings[] = {u8"Successed",
                                      u8"UrlInvalid",
                                      u8"TargetFilePathInvalid",
                                      u8"ThreadNumInvalid",
                                      u8"NetworkConnTimeoutInvalid",
                                      u8"NetworkReadTimeoutInvalid",
                                      u8"InternalNetworkError",
                                      u8"GenerateTargetFileFailed",
                                      u8"CleanupTmpFileFailed",
                                      u8"AlreadyDownloading",
                                      u8"Canceled",
                                      u8"CanceledAndUpdateIndexFailed",
                                      u8"Failed",
                                      u8"FailedAndUpdateIndexFailed"};
  return EnumStrings[enumVal];
}

class Teemo::TeemoImpl {
public:
  TeemoImpl() {}

public:
  std::shared_ptr<SliceManage> slice_manager;
  pplx::task<Result> result;
};

Teemo::Teemo() {
  impl_ = new TeemoImpl();

  impl_->slice_manager = std::make_shared<SliceManage>();
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

void Teemo::GlobalUnInit() { GlobalCurlInit(); }

void Teemo::SetVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept {
  impl_->slice_manager->SetVerboseOutput(verbose_functor);
}

void Teemo::SetSaveSliceFileToTempDir(bool enabled) noexcept {
  impl_->slice_manager->SetSaveSliceFileToTempDir(enabled);
}

bool Teemo::IsSaveSliceFileToTempDir() const noexcept {
  return impl_->slice_manager->IsSaveSliceFileToTempDir();
}

Result Teemo::SetThreadNum(size_t thread_num) noexcept {
  return impl_->slice_manager->SetThreadNum(thread_num);
}

size_t Teemo::GetThreadNum() const noexcept { return impl_->slice_manager->GetThreadNum(); }

utf8string Teemo::GetUrl() const noexcept { return impl_->slice_manager->GetUrl(); }

utf8string Teemo::GetTargetFilePath() const noexcept {
  return impl_->slice_manager->GetTargetFilePath();
}

Result Teemo::SetNetworkConnectionTimeout(size_t milliseconds) noexcept {
  return impl_->slice_manager->SetNetworkConnectionTimeout(milliseconds);
}

size_t Teemo::GetNetworkConnectionTimeout() const noexcept {
  return impl_->slice_manager->GetNetworkConnectionTimeout();
}

Result Teemo::SetNetworkReadTimeout(size_t milliseconds) noexcept {
  return impl_->slice_manager->SetNetworkReadTimeout(milliseconds);
}

size_t Teemo::GetNetworkReadTimeout() const noexcept {
  return impl_->slice_manager->GetNetworkReadTimeout();
}

void Teemo::SetSliceCacheExpiredTime(int seconds) noexcept {
  return impl_->slice_manager->SetSliceCacheExpiredTime(seconds);
}

int Teemo::GetSliceCacheExpiredTime() const noexcept {
  return impl_->slice_manager->GetSliceCacheExpiredTime();
}

void Teemo::SetMaxDownloadSpeed(size_t byte_per_seconds) noexcept {
  impl_->slice_manager->SetMaxDownloadSpeed(byte_per_seconds);
}

size_t Teemo::GetMaxDownloadSpeed() const noexcept {
  return impl_->slice_manager->GetMaxDownloadSpeed();
}

pplx::task<Result> Teemo::Start(const utf8string url, const utf8string &target_file_path,
                                ProgressFunctor progress_functor,
                                RealtimeSpeedFunctor realtime_speed_functor) noexcept {
  if (impl_->result._GetImpl() && !impl_->result.is_done())
    return pplx::task_from_result(AlreadyDownloading);

  impl_->result = pplx::task<Result>([=]() {
    Result result = impl_->slice_manager->Start(url, target_file_path, progress_functor,
                                                realtime_speed_functor);
    return pplx::task_from_result(result);
  });

  return impl_->result;
}

void Teemo::Stop(bool wait) noexcept {
  if (impl_->slice_manager)
    impl_->slice_manager->Stop();

  if (wait && impl_->result._GetImpl() && !impl_->result.is_done()) {
    impl_->result.wait();
  }
}
} // namespace teemo
