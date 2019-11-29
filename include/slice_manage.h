#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <future>
#include <condition_variable>
#include "slice.h"
#include "curl/curl.h"
#include "easy_file_download.h"

namespace easy_file_download {
    class Slice;
    class SliceManage : public std::enable_shared_from_this<SliceManage> {
      public:
        SliceManage();
        virtual ~SliceManage();

        std::string indexFilePath() const;
        std::string targetFilePath() const;

        Result Start(
            const std::string &url, 
            const std::string &target_file_path, 
            size_t thread_num, 
            ProgressFunctor progress_functor);
        void Stop();


        int GetThreadNum() const;
        std::string GetTargetFilePath() const;
        std::string GetIndexFilePath() const;
        std::string DumpSlicesInfo() const;
      protected:
        long QueryFileSize() const;
        bool LoadSlices(const std::string url, ProgressFunctor functor);
        bool CombineSlice();
        bool CleanupTmpFiles();
        bool UpdateIndexFile();
        void Destory();
        std::string GenerateIndexFilePath(const std::string &target_file_path) const;
      protected:
        std::string url_;
        std::string target_file_path_;
        std::string index_file_path_;
        int thread_num_;
        long file_size_;
        CURLM *multi_;
        ProgressFunctor progress_functor_;
        std::vector<std::shared_ptr<Slice>> slices_;
        std::future<void> progress_notify_thread_;

        bool stop_;
        std::mutex stop_mutex_;
        std::condition_variable stop_cond_var_;
    };
}