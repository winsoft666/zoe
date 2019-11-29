#pragma once
#include <string>
#include "pplx/pplxtasks.h"


#ifdef EFD_STATIC
    #define EFD_API
#else
    #if defined(EFD_EXPORTS)
        #if defined(_MSC_VER)
            #define EFD_API __declspec(dllexport)
        #else
            #define EFD_API
        #endif
    #else
        #if defined(_MSC_VER)
            #define EFD_API __declspec(dllimport)
        #else
            #define EFD_API
        #endif
    #endif
#endif

namespace easy_file_download {    
    enum Result {
        UrlInvalid = 0,
        TargetFilePathInvalid,
        ThreadNumInvalid,
        InternalNetworkError,
        CombineSliceFailed,
        CleanupTmpFileFailed,
        Downloading,
        Broken,
        BrokenAndUpdateIndexFailed,
        Success
    };
    EFD_API const char* GetResultString(int enumVal);

    typedef std::function< void(long total, long downloaded)> ProgressFunctor;
    typedef std::function<void(Result result)> FinishedFunctor;

    class EFD_API EasyFileDownload {
      public:
        EasyFileDownload();
        virtual ~EasyFileDownload();

        static void GlobalInit();
        static void GlobalUnInit();

        void SetThreadNum(size_t thread_num);
        void SetUrl(const std::string &url);
        void SetTargetFilePath(const std::string &file_path);
        void SetProgressFunctor(ProgressFunctor progress);

        pplx::task<Result> Start(
            size_t thread_num,
            const std::string url,
            const std::string &target_file_path,
            ProgressFunctor progress);

        pplx::task<Result> Start();

        void Stop(bool wait = false);

      protected:
        class EasyFileDownloadImpl;
        EasyFileDownloadImpl *impl_;
    };
}