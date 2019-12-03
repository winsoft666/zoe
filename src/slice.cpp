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

#include "slice.h"
#include <iostream>
#include <assert.h>
#include <string.h>
#include "file_util.h"
#include "curl_utils.h"

namespace easy_file_download {

    Slice::Slice(size_t index, std::shared_ptr<SliceManage> slice_manager) :
        index_(index)
        , slice_manager_(slice_manager)
        , file_(nullptr)
        , begin_(0)
        , end_(0)
        , capacity_(0)
        , origin_capacity_(0)
        , curl_(nullptr) {

    }

    Slice::~Slice() {
        if (file_) {
            fflush(file_);
            fclose(file_);
            file_ = nullptr;
        }
    }

    bool Slice::Init(const std::string &slice_file_path,
                     long begin,
                     long end,
                     long capacity) {
        begin_ = begin;
        end_ = end;
        capacity_ = capacity;

        bool need_generate_new_slice = true;
        do {
            if(slice_file_path.length() == 0)
                break;
            if (!FileIsRW(slice_file_path.c_str()))
                break;
            FILE *f = fopen(slice_file_path.c_str(), "a+b");
            if(!f)
                break;
            if (GetFileSize(f) != capacity_) {
                fclose(f);
                break;
            }
            fclose(f);
            need_generate_new_slice = false;
        } while (false);

        if(need_generate_new_slice) {
            remove(slice_file_path.c_str());
            file_path_ = GenerateSliceFilePath(index_, slice_manager_->GetTargetFilePath());
            remove(file_path_.c_str());
        } else {
            file_path_ = slice_file_path;
        }

        file_ = fopen(file_path_.c_str(), "a+b");
        fseek(file_, 0, SEEK_END);

        return true;
    }

    long Slice::begin() const {
        return begin_;
    }

    long Slice::end() const {
        return end_;
    }

    long Slice::capacity() const {
        return capacity_;
    }

    std::string Slice::filePath() const {
        return file_path_;
    }

    static size_t DownloadWriteCallback(char *buffer, size_t size, size_t nitems, void *outstream) {
        Slice *pThis = (Slice *)outstream;

        size_t write_size = size * nitems;
        size_t written = 0;

        FILE *f = pThis->GetFile();
        if (f) {
            written = fwrite(buffer, 1, write_size, f);
            fflush(f);
            pThis->IncreaseCapacity(written);
        }
        assert(write_size == written);
        if (write_size != written) {

        }

        return write_size;
    }

    bool Slice::InitCURL(CURLM *multi, size_t max_download_speed /* = 0*/) {
        curl_ = curl_easy_init();

        curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl_, CURLOPT_URL, slice_manager_->GetUrl().c_str());
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0);
        //if (ca_path_.length() > 0)
        //    curl_easy_setopt(curl_, CURLOPT_CAINFO, ca_path_.c_str());
        curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, 10L);
        curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, 30L);

        curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1);

        if (max_download_speed > 0) {
            curl_easy_setopt(curl_, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)max_download_speed);
        }

        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, DownloadWriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
        char range[64] = { 0 };
        if (begin_ + capacity_ >= 0 && end_ > 0 && end_ >= begin_ + capacity_)
            snprintf(range, sizeof(range), "%ld-%ld", begin_ + capacity_, end_);
        if (strlen(range) > 0) {
            CURLcode err = curl_easy_setopt(curl_, CURLOPT_RANGE, range);
            if (err != CURLE_OK) {
                std::cerr << "curl_easy_setopt CURLOPT_RANGE failed, code: " << err << std::endl;
                curl_easy_cleanup(curl_);
                curl_ = nullptr;
                return false;
            }
        }

        CURLMcode m_code = curl_multi_add_handle(multi, curl_);
        if (m_code != CURLE_OK) {
            curl_easy_cleanup(curl_);
            curl_ = nullptr;
            return false;
        }

        return true;
    }

    void Slice::UnInitCURL(CURLM *multi) {
        if (curl_) {
            if (multi) {
                CURLMcode code = curl_multi_remove_handle(multi, curl_);
                if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
                }
            }
            curl_easy_cleanup(curl_);
            curl_ = nullptr;
        }
    }

    bool Slice::AppendSelfToFile(FILE *f) {
        if (!f)
            return false;
        fflush(file_);
        fseek(file_, 0, SEEK_SET);
        char buf[1024];
        size_t read = 0;

        while ((read = fread(buf, 1, 1024, file_)) > 0) {
            size_t written = fwrite(buf, 1, read, f);
            assert(written == read);
            if (written != read)
                return false;
            read = 0;
        }

        return true;
    }

    bool Slice::RemoveFile() {
        if (file_) {
            fclose(file_);
            file_ = nullptr;
        }

        if (remove(file_path_.c_str()) != 0) {
            std::cerr << "remove file failed: " << file_path_ << std::endl;
            return false;
        }
        return true;
    }

    FILE *Slice::GetFile() {
        return file_;
    }

    void Slice::IncreaseCapacity(long i) {
        capacity_ += i;
    }

    std::string Slice::GenerateSliceFilePath(size_t index, const std::string &target_file_path) const {
        std::string target_dir;
        if (slice_manager_->IsEnableSaveSliceFileToTempDir()) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            char buf[MAX_PATH] = { 0 };
            DWORD ret_val = GetTempPathA(MAX_PATH, buf);
            if (ret_val > 0 && ret_val < MAX_PATH) {
                target_dir = buf;
            }
#else
            target_dir = "/var/tmp/";
#endif
        }
        if(target_dir.length() == 0)
            target_dir = GetDirectory(target_file_path);

        std::string target_filename = GetFileName(target_file_path);

        std::string slice_filename = target_filename + ".edf" + std::to_string(index);
        return AppendFileName(target_dir, slice_filename);
    }
}