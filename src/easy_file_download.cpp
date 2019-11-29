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
            "UrlInvalid",
            "TargetFilePathInvalid",
            "ThreadNumInvalid",
            "InternalNetworkError",
            "CombineSliceFailed",
            "CleanupTmpFileFailed",
            "Downloading",
            "Broken",
            "BrokenAndUpdateIndexFailed",
            "Success"
        };
        return EnumStrings[enumVal];
    }

    class EasyFileDownload::EasyFileDownloadImpl {
      public:
        EasyFileDownloadImpl() :
            thread_num(0) {

        }

      public:
        bool enable_save_slice_file_to_tmp_dir;
        size_t thread_num;
        std::string url;
        std::string target_file_path;
        ProgressFunctor progress_functor;
        RealtimeSpeedFunctor realtime_speed_functor;
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
        GlobalCurlInit();
    }

    void EasyFileDownload::GlobalUnInit() {
        GlobalCurlInit();
    }

    void EasyFileDownload::SetEnableSaveSliceFileToTempDir(bool enabled) {
        impl_->enable_save_slice_file_to_tmp_dir = enabled;
    }

    void EasyFileDownload::SetThreadNum(size_t thread_num) {
        impl_->thread_num = thread_num;
    }

    void EasyFileDownload::SetUrl(const std::string &url) {
        impl_->url = url;
    }

    void EasyFileDownload::SetTargetFilePath(const std::string &file_path) {
        impl_->target_file_path = file_path;
    }

    void EasyFileDownload::SetProgressFunctor(ProgressFunctor progress_functor) {
        impl_->progress_functor = progress_functor;
    }

    void EasyFileDownload::SetRealtimeSpeedFunctor(RealtimeSpeedFunctor realtime_speed_functor) {
        impl_->realtime_speed_functor = realtime_speed_functor;
    }

    pplx::task<Result> EasyFileDownload::Start(
        bool enable_save_slice_to_tmp,
        size_t thread_num,
        const std::string url,
        const std::string &target_file_path,
        ProgressFunctor progress_functor,
        RealtimeSpeedFunctor realtime_speed_functor
    ) {
        impl_->enable_save_slice_file_to_tmp_dir = enable_save_slice_to_tmp;
        impl_->thread_num = thread_num;
        impl_->url = url;
        impl_->target_file_path = target_file_path;
        impl_->progress_functor = progress_functor;
        impl_->realtime_speed_functor = realtime_speed_functor;

        return Start();
    }

    pplx::task<Result> EasyFileDownload::Start() {
        if (impl_->result._GetImpl() && !impl_->result.is_done())
            return pplx::task_from_result(Result::AlreadyDownloading);

        impl_->result = pplx::task<Result>([this]() {
            Result result = impl_->slice_manager->Start(
                                impl_->url,
                                impl_->target_file_path,
                                impl_->enable_save_slice_file_to_tmp_dir,
                                impl_->thread_num,
                                impl_->progress_functor,
                                impl_->realtime_speed_functor
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