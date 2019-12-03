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
#include <algorithm>
#include <sstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include "file_util.h"
#include "curl_utils.h"

using json = nlohmann::json;

#define INDEX_FILE_SIGN_STRING "EASY-FILE-DOWNLOAD"

namespace easy_file_download {

    SliceManage::SliceManage() :
        multi_(nullptr)
        , enable_save_slice_to_tmp_dir_(false)
        , thread_num_(1)
        , slice_cache_expired_seconds_(-1)
        , network_conn_timeout_(3000)
        , network_read_timeout_(3000)
        , max_download_speed_(0)
        , stop_(true)
        , file_size_(-1) {
    }

    SliceManage::~SliceManage() {

    }

    Result SliceManage::SetNetworkConnectionTimeout(size_t conn_timeout_ms) {
        if (!stop_)
            return AlreadyDownloading;
        if (conn_timeout_ms == 0)
            return NetworkConnTimeoutInvalid;

        network_conn_timeout_ = conn_timeout_ms;
        return Successed;
    }

    size_t SliceManage::GetNetworkConnectionTimeout() const {
        return network_conn_timeout_;
    }

    Result SliceManage::SetNetworkReadTimeout(size_t read_timeout_ms) {
        if (!stop_)
            return AlreadyDownloading;
        if (read_timeout_ms == 0)
            return NetworkReadTimeoutInvalid;

        network_read_timeout_ = read_timeout_ms;
        return Successed;
    }

    size_t SliceManage::GetNetworkReadTimeout() const {
        return network_read_timeout_;
    }

    void SliceManage::SetSliceCacheExpiredTime(int seconds) {
        slice_cache_expired_seconds_ = seconds;
    }

    int SliceManage::GetSliceCacheExpiredTime() const {
        return slice_cache_expired_seconds_;
    }

    Result SliceManage::SetThreadNum(size_t thread_num) {
        if (!stop_)
            return AlreadyDownloading;
        if (thread_num == 0 || thread_num > 100)
            return ThreadNumInvalid;

        thread_num_ = thread_num;
        return Successed;
    }

    size_t SliceManage::GetThreadNum() const {
        return thread_num_;
    }

    void SliceManage::SetEnableSaveSliceFileToTempDir(bool enabled) {
        enable_save_slice_to_tmp_dir_ = enabled;
    }

    bool SliceManage::IsEnableSaveSliceFileToTempDir() const {
        return enable_save_slice_to_tmp_dir_;
    }

    void SliceManage::SetMaxDownloadSpeed(size_t byte_per_seconds) {
        max_download_speed_ = byte_per_seconds;
    }

    size_t SliceManage::GetMaxDownloadSpeed() const {
        return max_download_speed_;
    }

    Result SliceManage::Start(
        const std::string &url,
        const std::string &target_file_path,
        ProgressFunctor progress_functor,
        RealtimeSpeedFunctor realtime_speed_functor) {
        if (url.length() == 0)
            return Result::UrlInvalid;

        // be sure previous threads has exit
        if (progress_notify_thread_.valid())
            progress_notify_thread_.wait();
        if (speed_notify_thread_.valid())
            speed_notify_thread_.wait();

        stop_ = false;

        url_ = url;
        progress_functor_ = progress_functor;
        speed_functor_ = realtime_speed_functor;
        target_file_path_ = target_file_path;
        index_file_path_ = GenerateIndexFilePath(target_file_path);

        bool valid_resume = false;
        do {
            if (!FileIsRW(index_file_path_.c_str()))   break;
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
                thread_num_ = std::min(file_size_, (long)thread_num_);
                long each_slice_size = file_size_ / thread_num_;
                for (size_t i = 0; i < thread_num_; i++) {
                    long end = (i == thread_num_ - 1) ? file_size_ - 1 : ((i + 1) * each_slice_size) - 1;
                    std::shared_ptr<Slice> slice = std::make_shared<Slice>(i, shared_from_this());
                    slice->Init("", i * each_slice_size, end, 0);
                    slices_.push_back(slice);
                }
            } else if (file_size_ == 0) {
                FILE *f = fopen(target_file_path_.c_str(), "wb");
                if (!f)
                    return GenerateTargetFileFailed;
                fclose(f);
                Destory();
                return Successed;
            } else {
                thread_num_ = 1;
                std::shared_ptr<Slice> slice = std::make_shared<Slice>(0, shared_from_this());
                slice->Init("", 0, -1, 0);
                slices_.push_back(slice);
            }
        }

        std::cout << std::endl << "Dump Slices:" << std::endl << DumpSlicesInfo() << std::endl;

        multi_ = curl_multi_init();
        if (!multi_) {
            Destory();
            return Result::InternalNetworkError;
        }

        size_t uncomplete_slice_num = 0;
        for (auto &slice : slices_) {
            if (!slice->IsDownloadCompleted())
                uncomplete_slice_num++;
        }
        thread_num_ = uncomplete_slice_num;
        size_t each_slice_download_speed = max_download_speed_ / uncomplete_slice_num;

        bool init_curl_ret = true;
        for (auto &slice : slices_) {
            if (!slice->IsDownloadCompleted()) {
                if (!slice->InitCURL(multi_, each_slice_download_speed)) {
                    init_curl_ret = false;
                    break;
                }
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
        size_t thread_num_copy = thread_num_;

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


            /* On success the value of maxfd is guaranteed to be >= -1. We call
                select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
                no fds ready yet so we sleep 100ms, which is the minimum suggested value in the
                curl_multi_fdset() doc. 
            */
            int rc;
            if (maxfd == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                rc = 0;
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

        } while (still_running);

        /* See how the transfers went */
        size_t done_thread = 0;
        CURLMsg *msg = NULL;
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

        Result ret;
        do {
            if (done_thread == thread_num_copy) {
                if (file_size_ == -1 || (file_size_ > 0 && total_capacity == file_size_)) {
                    if (!CombineSlice()) {
                        ret = GenerateTargetFileFailed;
                        break;
                    }

                    if (!CleanupTmpFiles()) {
                        ret = CleanupTmpFileFailed;
                        break;
                    }

                    if (progress_functor_)
                        progress_functor_(file_size_, total_capacity);

                    ret = Successed;
                    break;
                }
            }

            if (stop_) {
                ret = UpdateIndexFile() ? Canceled : CanceledAndUpdateIndexFailed;
                break;
            }

            ret = UpdateIndexFile() ? Failed : FailedAndUpdateIndexFailed;
        } while (false);

        Destory();

        return ret;
    }

    void SliceManage::Stop() {
        std::lock_guard<std::mutex> lg(stop_mutex_);
        stop_ = true;
    }

    std::string SliceManage::GetTargetFilePath() const {
        return target_file_path_;
    }

    std::string SliceManage::GetIndexFilePath() const {
        return index_file_path_;
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
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, network_conn_timeout_); // Time-out connect operations after this amount of seconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, network_read_timeout_); // Time-out the read operation after this amount of seconds

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
            if (http_code != 200 &&
                    // A 350 response code is sent by the server in response to a file-related command that
                    // requires further commands in order for the operation to be completed
                    http_code != 350
               ) {
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
            std::string str_sign(file_content.data(), strlen(INDEX_FILE_SIGN_STRING));
            if (str_sign != INDEX_FILE_SIGN_STRING)
                return false;

            std::string str_json(file_content.data() + 18);
            json j = json::parse(str_json);

            time_t last_update_time = j["update_time"].get<time_t>();
            if (slice_cache_expired_seconds_ >= 0) {
                time_t now = time(nullptr);
                if (now - last_update_time > slice_cache_expired_seconds_)
                    return false;
            }

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
        if (FileIsExist(index_file_path_.c_str()) && remove(index_file_path_.c_str()) != 0) {
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
        j["update_time"] = time(nullptr);
        j["file_size"] = file_size_;
        j["url"] = url_;
        json s;
        for (auto slice : slices_) {
            s.push_back({ {"path", slice->filePath()}, {"begin", slice->begin()}, {"end", slice->end()}, {"capacity", slice->capacity()} });
        }
        j["slices"] = s;
        std::string str_json = j.dump();
        fwrite(INDEX_FILE_SIGN_STRING, 1, strlen(INDEX_FILE_SIGN_STRING), f);
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