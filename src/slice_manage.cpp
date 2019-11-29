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

#include "slice_manage.h"
#include <array>
#include <io.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include "file_util.h"
#include "curl_utils.h"

using json = nlohmann::json;

namespace easy_file_download {

    SliceManage::SliceManage() :
        multi_(nullptr)
        , stop_(false)
        , file_size_(-1) {
    }

    SliceManage::~SliceManage() {

    }

    std::string SliceManage::indexFilePath() const {
        return index_file_path_;
    }

    std::string SliceManage::targetFilePath() const {
        return target_file_path_;
    }

    Result SliceManage::Start(
        const std::string &url,
        const std::string &target_file_path, 
        bool enable_save_slice_to_tmp_dir,
        size_t thread_num, 
        ProgressFunctor progress_functor,
        RealtimeSpeedFunctor realtime_speed_functor) {
        if (url.length() == 0)
            return Result::UrlInvalid;

        if (thread_num == 0 || thread_num > 80)
            return Result::ThreadNumInvalid;

        // be sure previous thread has exit
        if (progress_notify_thread_.valid())
            progress_notify_thread_.wait();
        if (speed_notify_thread_.valid())
            speed_notify_thread_.wait();

        stop_ = false;

        url_ = url;
        progress_functor_ = progress_functor;
        speed_functor_ = realtime_speed_functor;
        target_file_path_ = target_file_path;
        enable_save_slice_to_tmp_dir_ = enable_save_slice_to_tmp_dir;
        index_file_path_ = GenerateIndexFilePath(target_file_path);

        bool valid_resume = false;
        do {
            if (access(index_file_path_.c_str(), 0) != 0)   break;
            if (!LoadSlices(url_, progress_functor)) {
                remove(index_file_path_.c_str());
                break;
            }
            if (file_size_ == -1)        break;
            if (slices_.size() == 0)     break;
            valid_resume = true;
        } while (false);

        if (!valid_resume) {
            file_size_ = QueryFileSize();
            slices_.clear();

            if (file_size_ > 0) {
                thread_num = std::min(file_size_, (long)thread_num);
                long each_slice_size = file_size_ / thread_num;
                for (size_t i = 0; i < thread_num; i++) {
                    long end = (i == thread_num - 1) ? file_size_ - 1 : ((i + 1) * each_slice_size) - 1;
                    std::shared_ptr<Slice> slice = std::make_shared<Slice>(i, shared_from_this());
                    slice->Init("",
                        i * each_slice_size,
                        end,
                        0);
                    slices_.push_back(slice);
                }
            } else if (file_size_ == 0) {
            } else {
                thread_num = 1;
                std::shared_ptr<Slice> slice = std::make_shared<Slice>(0, shared_from_this());
                slice->Init("",
                    0,
                    -1,
                    0);
                slices_.push_back(slice);
            }
        }

        std::cout << std::endl << "Dump Slices:" << std::endl << DumpSlicesInfo() << std::endl;

        thread_num_ = thread_num;

        multi_ = curl_multi_init();
        if (!multi_) {
            Destory();
            return Result::InternalNetworkError;
        }

        bool init_curl_ret = true;
        for (auto &slice : slices_) {
            if (!slice->InitCURL(multi_)) {
                init_curl_ret = false;
                break;
            }
        }

        if (!init_curl_ret) {
            Destory();
            return Result::InternalNetworkError;
        }

        progress_notify_thread_ = std::async(std::launch::async, [this]() {
            while (true) {
                {
                    std::unique_lock<std::mutex> ul(stop_mutex_);
                    stop_cond_var_.wait_for(ul, std::chrono::milliseconds(500), [this] {return stop_; });
                    if (stop_)
                        return;
                }

                long downloaded = 0;
                for (const auto &s : slices_) {
                    downloaded += s->capacity();
                }

                if (progress_functor_)
                    progress_functor_(file_size_, downloaded);
            }
        });

        long init_total_capacity = 0;
        for (const auto &s : slices_)
            init_total_capacity += s->capacity();

        speed_notify_thread_ = std::async(std::launch::async, [this, init_total_capacity]() {
            while (true) {
                {
                    std::unique_lock<std::mutex> ul(stop_mutex_);
                    stop_cond_var_.wait_for(ul, std::chrono::milliseconds(1000), [this] {return stop_; });
                    if (stop_)
                        return;
                }

                long now = 0;
                for (const auto &s : slices_)
                    now += s->capacity();

                static long last = init_total_capacity;
                if (now >= last) {
                    long downloaded = now - last;
                    last = now;
                    if (speed_functor_)
                        speed_functor_(downloaded);
                }
            }
        });

        int still_running = 0;
        CURLMcode m_code = curl_multi_perform(multi_, &still_running);

        do {
            {
                std::lock_guard<std::mutex> lg(stop_mutex_);
                if(stop_)
                    break;
            }

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            long curl_timeo = -1;
            curl_multi_timeout(multi_, &curl_timeo);

            if (curl_timeo > 0) {
                timeout.tv_sec = curl_timeo / 1000;
                if (timeout.tv_sec > 1)
                    timeout.tv_sec = 1;
                else
                    timeout.tv_usec = (curl_timeo % 1000) * 1000;
            } else {
                timeout.tv_sec = 0;
                timeout.tv_usec = 100 * 1000;
            }

            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = -1;

            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            CURLMcode code = curl_multi_fdset(multi_, &fdread, &fdwrite, &fdexcep, &maxfd);
            if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
                break;
            }

            int rc;
            if (maxfd == -1) {
#ifdef _WIN32
                Sleep(100);
                rc = 0;
#else
                /* Portable sleep for platforms other than Windows. */
                struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
                rc = select(0, NULL, NULL, NULL, &wait);
#endif
            } else {
                rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
            }

            switch (rc) {
            case -1:
                break; /* select error */
            case 0: /* timeout */
            default: /* action */
                curl_multi_perform(multi_, &still_running);
                break;
            }

#if (defined _WIN32 || defined WIN32)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
#else
            struct timeval wait = { 0, 10 * 1000 }; /* 100ms */
            rc = select(0, NULL, NULL, NULL, &wait);
#endif
        } while (still_running);

        /* See how the transfers went */
        size_t done_thread = 0;
        CURLMsg *msg;  /* for picking up messages with the transfer status */
        int msgsInQueue;
        while ((msg = curl_multi_info_read(multi_, &msgsInQueue)) != NULL) {
            if (msg->msg == CURLMSG_DONE) {
                if (msg->data.result == CURLE_OK) {
                    done_thread++;
                }
            }
        }

        long total_capacity = 0;
        for (auto slice : slices_)
            total_capacity += slice->capacity();

        if (done_thread != thread_num_ && total_capacity != file_size_) {
            Result ret = UpdateIndexFile() ? Result::Broken : Result::BrokenAndUpdateIndexFailed;
            std::cout << std::endl << "Dump Slices:" << std::endl << DumpSlicesInfo() << std::endl;
            Destory();
            return ret;
        }

        if (!CombineSlice()) {
            Destory();
            return Result::CombineSliceFailed;
        }

        if (!CleanupTmpFiles()) {
            Destory();
            return Result::CleanupTmpFileFailed;
        }

        if (progress_functor_)
            progress_functor_(file_size_, file_size_);

        {
            std::lock_guard<std::mutex> lg(stop_mutex_);
            stop_ = true;
        }

        return Result::Success;
    }

    void SliceManage::Stop() {
        std::lock_guard<std::mutex> lg(stop_mutex_);
        stop_ = true;
    }

    bool SliceManage::IsEnableSaveSliceFileToTmpDir() const {
        return enable_save_slice_to_tmp_dir_;
    }

    std::string SliceManage::GetTargetFilePath() const {
        return target_file_path_;
    }

    std::string SliceManage::GetIndexFilePath() const {
        return index_file_path_;
    }

    int SliceManage::GetThreadNum() const {
        return thread_num_;
    }

    std::string SliceManage::GetUrl() const {
        return url_;
    }

    std::string SliceManage::DumpSlicesInfo() const {
        std::stringstream ss_dump;
        for (auto &s : slices_) {
            ss_dump << s->filePath() << ": " << s->begin() << " ~ " << s->end() << ": " << s->capacity() << std::endl;
        }

        return ss_dump.str();
    }

    static size_t QueryFileSizeCallback(char *buffer, size_t size, size_t nitems, void *outstream) {
        size_t avaliable = size * nitems;
        return (avaliable);
    }

    long SliceManage::QueryFileSize() const {
        ScopedCurl scoped_curl;
        CURL *curl = scoped_curl.GetCurl();

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADER, 1);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, ca_path_.length() > 0);
        //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, ca_path_.length() > 0);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3); // Time-out connect operations after this amount of seconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2); // Time-out the read operation after this amount of seconds

        //if (ca_path_.length() > 0)
        //    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path_.c_str());

        // avoid libcurl failed with "Failed writing body".
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, QueryFileSizeCallback);

        CURLcode ret_code = curl_easy_perform(curl);
        if (ret_code != CURLE_OK) {
            return -1;
        }

        int http_code = 0;
        ret_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (ret_code == CURLE_OK) {
            if (http_code != 200) {
                return -1;
            }
        } else {
            return -1;
        }

        int64_t file_size = 0;
        ret_code = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &file_size);
        if (ret_code != CURLE_OK) {
            return -1;
        }
        return file_size;
    }

    bool SliceManage::LoadSlices(const std::string url, ProgressFunctor functor) {
        FILE *file = fopen(index_file_path_.c_str(), "rb");
        if (!file)
            return false;
        long file_size = GetFileSize(file);
        fseek(file, 0, SEEK_SET);
        std::vector<char> file_content(file_size + 1, 0);
        fread(file_content.data(), 1, file_size, file);
        fclose(file);

        try {
            std::string str_json(file_content.data());
            json j = json::parse(str_json);
            file_size_ = j["file_size"];
            if (j["url"].get<std::string>() != url)
                return false;
            for (auto &it : j["slices"]) {
                std::shared_ptr<Slice> slice = std::make_shared<Slice>(9999, shared_from_this());
                slice->Init( 
                    it["path"].get<std::string>(), it["begin"].get<long>(), it["end"].get<long>(), it["capacity"].get<long>());
                slices_.push_back(slice);
            }
        } catch (const std::exception &e) {
            if(e.what())
                std::cerr << e.what() << std::endl;
            file_size_ = -1;
            slices_.clear();
            return false;
        }
        return true;
    }

    bool sliceLessBegin(const std::shared_ptr<Slice> &s1, const std::shared_ptr<Slice> &s2) {
        return s1->begin() < s2->begin();
    }

    bool SliceManage::CombineSlice() {
        std::sort(slices_.begin(), slices_.end(), sliceLessBegin);

        FILE *f = fopen(target_file_path_.c_str(), "wb");
        if (!f)
            return false;
        for (auto slice : slices_) {
            if (!slice->AppendSelfToFile(f)) {
                fclose(f);
                return false;
            }
        }
        fclose(f);
        return true;
    }

    bool SliceManage::CleanupTmpFiles() {
        bool ret = true;
        if (access(index_file_path_.c_str(), 0) == 0 && remove(index_file_path_.c_str()) != 0) {
            std::cerr << "remove file failed: " << index_file_path_ << std::endl;
            ret = false;
        }
        for (auto slice : slices_) {
            if (!slice->RemoveFile()) {
                ret = false;
            }
        }
        return ret;
    }

    bool SliceManage::UpdateIndexFile() {
        FILE *f = fopen(index_file_path_.c_str(), "wb");
        if (!f)
            return false;
        json j;
        j["file_size"] = file_size_;
        j["url"] = url_;
        json s;
        for (auto slice : slices_) {
            s.push_back({ {"path", slice->filePath()}, {"begin", slice->begin()}, {"end", slice->end()}, {"capacity", slice->capacity()} });
        }
        j["slices"] = s;
        std::string str_json = j.dump();
        fwrite(str_json.c_str(), 1, str_json.size(), f);
        fclose(f);

        return true;
    }

    void SliceManage::Destory() {
        for (auto s : slices_)
            s->UnInitCURL(multi_);

        if (multi_) {
            curl_multi_cleanup(multi_);
            multi_ = nullptr;
        }

        slices_.clear();

        {
            std::lock_guard<std::mutex> lg(stop_mutex_);
            stop_ = true;
        }
    }

    std::string SliceManage::GenerateIndexFilePath(const std::string &target_file_path) const {
        std::string target_dir = GetDirectory(target_file_path);
        std::string target_filename = GetFileName(target_file_path);

        std::string indexfilename = target_filename + ".efdindex";
        return AppendFileName(target_dir, indexfilename);
    }
}