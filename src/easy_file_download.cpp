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

#include "easy_file_download.h"
#include "file_util.h"
#include "curl_utils.h"
#include "slice_manage.h"

namespace easy_file_download {
    const char *GetResultString(int enumVal) {
        static const char *EnumStrings[] = {
            "Successed",
            "UrlInvalid",
            "TargetFilePathInvalid",
            "ThreadNumInvalid",
            "NetworkConnTimeoutInvalid",
            "NetworkReadTimeoutInvalid",
            "InternalNetworkError",
            "GenerateTargetFileFailed",
            "CleanupTmpFileFailed",
            "AlreadyDownloading",
            "Canceled",
            "CanceledAndUpdateIndexFailed",
            "Failed",
            "FailedAndUpdateIndexFailed"
        };
        return EnumStrings[enumVal];
    }

    class EasyFileDownload::EasyFileDownloadImpl {
      public:
        EasyFileDownloadImpl() {

        }

      public:
        std::shared_ptr<SliceManage> slice_manager;
        pplx::task<Result> result;
    };

    EasyFileDownload::EasyFileDownload() {
        impl_ = std::make_unique<EasyFileDownloadImpl>();

        impl_->slice_manager = std::make_shared<SliceManage>();
    }

    EasyFileDownload::~EasyFileDownload() {
    }

    void EasyFileDownload::GlobalInit() {
        static bool has_init = false;
        if (!has_init) {
            has_init = true;
            GlobalCurlInit();
        }
    }

    void EasyFileDownload::GlobalUnInit() {
        GlobalCurlInit();
    }

    void EasyFileDownload::SetEnableSaveSliceFileToTempDir(bool enabled) {
        impl_->slice_manager->SetEnableSaveSliceFileToTempDir(enabled);
    }

    bool EasyFileDownload::IsEnableSaveSliceFileToTempDir() const {
        return impl_->slice_manager->IsEnableSaveSliceFileToTempDir();
    }

    Result EasyFileDownload::SetThreadNum(size_t thread_num) {
        return impl_->slice_manager->SetThreadNum(thread_num);
    }

    size_t EasyFileDownload::GetThreadNum() const {
        return impl_->slice_manager->GetThreadNum();
    }

    std::string EasyFileDownload::GetUrl() const {
        return impl_->slice_manager->GetUrl();
    }

    std::string EasyFileDownload::GetTargetFilePath() const {
        return impl_->slice_manager->GetTargetFilePath();
    }

    Result EasyFileDownload::SetNetworkConnectionTimeout(size_t milliseconds) {
        return impl_->slice_manager->SetNetworkConnectionTimeout(milliseconds);
    }

    size_t EasyFileDownload::GetNetworkConnectionTimeout() const {
        return impl_->slice_manager->GetNetworkConnectionTimeout();
    }

    Result EasyFileDownload::SetNetworkReadTimeout(size_t milliseconds) {
        return impl_->slice_manager->SetNetworkReadTimeout(milliseconds);
    }

    size_t EasyFileDownload::GetNetworkReadTimeout() const {
        return impl_->slice_manager->GetNetworkReadTimeout();
    }

    void EasyFileDownload::SetSliceCacheExpiredTime(int seconds) {
        return impl_->slice_manager->SetSliceCacheExpiredTime(seconds);
    }

    int EasyFileDownload::GetSliceCacheExpiredTime() const {
        return impl_->slice_manager->GetSliceCacheExpiredTime();
    }

    void EasyFileDownload::SetMaxDownloadSpeed(size_t byte_per_seconds) {
        impl_->slice_manager->SetMaxDownloadSpeed(byte_per_seconds);
    }

    size_t EasyFileDownload::GetMaxDownloadSpeed() const {
        return impl_->slice_manager->GetMaxDownloadSpeed();
    }

    pplx::task<Result> EasyFileDownload::Start(
        const std::string url,
        const std::string &target_file_path,
        ProgressFunctor progress_functor,
        RealtimeSpeedFunctor realtime_speed_functor
    ) {
        if (impl_->result._GetImpl() && !impl_->result.is_done())
            return pplx::task_from_result(AlreadyDownloading);

        impl_->result = pplx::task<Result>([ = ]() {
            Result result = impl_->slice_manager->Start(
                                url,
                                target_file_path,
                                progress_functor,
                                realtime_speed_functor
                            );
            return pplx::task_from_result(result);
        });

        return impl_->result;
    }

    void EasyFileDownload::Stop(bool wait) {
        if (impl_->slice_manager)
            impl_->slice_manager->Stop();

        if (wait && !impl_->result.is_done()) {
            impl_->result.wait();
        }
    }
}