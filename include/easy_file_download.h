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
#ifndef EFD_EASY_FILE_DOWNLOAD_H_
#define EFD_EASY_FILE_DOWNLOAD_H_
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
        Successed = 0,
        UrlInvalid,
        TargetFilePathInvalid,
        ThreadNumInvalid,
        NetworkConnTimeoutInvalid,
        NetworkReadTimeoutInvalid,
        InternalNetworkError,
        GenerateTargetFileFailed,
        CleanupTmpFileFailed,
        AlreadyDownloading,
        Canceled,
        CanceledAndUpdateIndexFailed,
        Failed,
        FailedAndUpdateIndexFailed,
    };
    EFD_API const char* GetResultString(int enumVal);

    typedef std::function< void(long total, long downloaded)> ProgressFunctor;
    typedef std::function<void(long byte_per_sec)> RealtimeSpeedFunctor;

    class EFD_API EasyFileDownload {
      public:
        EasyFileDownload();
        virtual ~EasyFileDownload();

        static void GlobalInit();
        static void GlobalUnInit();

        void SetEnableSaveSliceFileToTempDir(bool enabled);
        bool IsEnableSaveSliceFileToTempDir() const;

        Result SetThreadNum(size_t thread_num);
        size_t GetThreadNum() const;

        std::string GetUrl() const;
        std::string GetTargetFilePath() const;

        Result SetNetworkConnectionTimeout(size_t milliseconds); // default is 3000ms
        size_t GetNetworkConnectionTimeout() const;

        Result SetNetworkReadTimeout(size_t milliseconds); // default is 3000ms
        size_t GetNetworkReadTimeout() const;

        void SetSliceCacheExpiredTime(int seconds); // default is -1 = forever, 0 = not use exist slice cache
        int GetSliceCacheExpiredTime() const;

        void SetMaxDownloadSpeed(size_t byte_per_seconds); // default is 0 = not limit
        size_t GetMaxDownloadSpeed() const;


        pplx::task<Result> Start(
            const std::string url,
            const std::string &target_file_path,
            ProgressFunctor progress_functor,
            RealtimeSpeedFunctor realtime_speed_functor);


        void Stop(bool wait = false);

      protected:
        class EasyFileDownloadImpl;
        EasyFileDownloadImpl *impl_;
    };
}

#endif