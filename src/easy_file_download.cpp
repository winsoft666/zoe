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
            "Broken",
            "BrokenAndUpdateIndexFailed",
        };
        return EnumStrings[enumVal];
    }

    class EasyFileDownload::EasyFileDownloadImpl {
      public:
        EasyFileDownloadImpl() :
            thread_num(1)
            , network_conn_timeout(3000)
            , network_read_timeout(3000) {

        }

      public:
        bool enable_save_slice_file_to_tmp_dir;
        size_t thread_num;
        size_t network_conn_timeout;
        size_t network_read_timeout;
        std::shared_ptr<SliceManage> slice_manager;
        pplx::task<Result> result;
    };

    EasyFileDownload::EasyFileDownload() : impl_(new EasyFileDownloadImpl()) {
        impl_->slice_manager = std::make_shared<SliceManage>();
    }

    EasyFileDownload::~EasyFileDownload() {
        delete impl_;
        impl_ = nullptr;
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
        impl_->enable_save_slice_file_to_tmp_dir = enabled;
    }

    bool EasyFileDownload::IsEnableSaveSliceFileToTempDir() const {
        return impl_->enable_save_slice_file_to_tmp_dir;
    }

    void EasyFileDownload::SetThreadNum(size_t thread_num) {
        impl_->thread_num = thread_num;
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

    void EasyFileDownload::SetNetworkConnectionTimeout(size_t milliseconds) {
        impl_->network_conn_timeout = milliseconds;
    }

    size_t EasyFileDownload::GetNetworkConnectionTimeout() const {
        return impl_->network_conn_timeout;
    }

    void EasyFileDownload::SetNetworkReadTimeout(size_t milliseconds) {
        impl_->network_read_timeout = milliseconds;
    }

    size_t EasyFileDownload::GetNetworkReadTimeout() const {
        return impl_->network_read_timeout;
    }

    pplx::task<Result> EasyFileDownload::Start(
        const std::string url,
        const std::string &target_file_path,
        ProgressFunctor progress_functor,
        RealtimeSpeedFunctor realtime_speed_functor
    ) {
        if (impl_->result._GetImpl() && !impl_->result.is_done())
            return pplx::task_from_result(Result::AlreadyDownloading);

        Result network_res = impl_->slice_manager->SetNetworkTimeout(impl_->network_conn_timeout, impl_->network_read_timeout);
        if (network_res != Result::Successed)
            return pplx::task_from_result(network_res);

        impl_->result = pplx::task<Result>([ = ]() {
            Result result = impl_->slice_manager->Start(
                                url,
                                target_file_path,
                                impl_->enable_save_slice_file_to_tmp_dir,
                                impl_->thread_num,
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