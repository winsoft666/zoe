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
        UrlInvalid = 0,
        TargetFilePathInvalid,
        ThreadNumInvalid,
        InternalNetworkError,
        CombineSliceFailed,
        CleanupTmpFileFailed,
        AlreadyDownloading,
        Broken,
        BrokenAndUpdateIndexFailed,
        Successed
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
        void SetThreadNum(size_t thread_num);
        void SetUrl(const std::string &url);
        void SetTargetFilePath(const std::string &file_path);
        void SetProgressFunctor(ProgressFunctor progress_functor);
        void SetRealtimeSpeedFunctor(RealtimeSpeedFunctor realtime_speed_functor);

        pplx::task<Result> Start(
            bool enable_save_slice_to_tmp,
            size_t thread_num,
            const std::string url,
            const std::string &target_file_path,
            ProgressFunctor progress_functor,
            RealtimeSpeedFunctor realtime_speed_functor);

        pplx::task<Result> Start();

        void Stop(bool wait = false);

      protected:
        class EasyFileDownloadImpl;
        EasyFileDownloadImpl *impl_;
    };
}

#endif