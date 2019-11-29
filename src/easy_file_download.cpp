#include "easy_file_download.h"
#include "file_util.h"
#include "curl_utils.h"
#include "slice_manage.h"

namespace easy_file_download {
    const char* GetResultString(int enumVal) {
        static const char * EnumStrings[] = {
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
        size_t thread_num;
        std::string url;
        std::string target_file_path;
        ProgressFunctor progress_functor;
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

    void EasyFileDownload::SetThreadNum(size_t thread_num) {
        impl_->thread_num = thread_num;
    }

    void EasyFileDownload::SetUrl(const std::string &url) {
        impl_->url = url;
    }

    void EasyFileDownload::SetTargetFilePath(const std::string &file_path) {
        impl_->target_file_path = file_path;
    }

    void EasyFileDownload::SetProgressFunctor(ProgressFunctor progress) {
        impl_->progress_functor = progress;
    }

    pplx::task<Result> EasyFileDownload::Start(
        size_t thread_num,
        const std::string url,
        const std::string &target_file_path,
        ProgressFunctor progress
    ) {
        impl_->thread_num = thread_num;
        impl_->url = url;
        impl_->target_file_path = target_file_path;
        impl_->progress_functor = progress;

        return Start();
    }

    pplx::task<Result> EasyFileDownload::Start() {
        if (impl_->result._GetImpl() && !impl_->result.is_done())
            return pplx::task_from_result(Result::Downloading);
        impl_->result = pplx::task<Result>([this]() {
            Result result = impl_->slice_manager->Start(
                                impl_->url,
                                impl_->target_file_path,
                                impl_->thread_num,
                                impl_->progress_functor
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