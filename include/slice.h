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

#ifndef EFD_SLICE_H__
#define EFD_SLICE_H__
#pragma once

#include <string>
#include <mutex>
#include <memory>
#include "slice_manage.h"
#include "curl/curl.h"
#include "easy_file_download.h"

namespace easy_file_download {
    class SliceManage;
    class Slice {
      public:
        Slice(size_t index, std::shared_ptr<SliceManage> slice_manager);
        virtual ~Slice();

        bool Init(const std::string &slice_file_path,
                  long begin,
                  long end,
                  long capacity);

        long begin() const;
        long end() const;
        long capacity() const;
        std::string filePath() const;

        bool InitCURL(CURLM *multi);
        void UnInitCURL(CURLM *multi);

        bool AppendSelfToFile(FILE *f);

        bool RemoveFile();

      protected:
        std::string GenerateSliceFilePath(size_t index, const std::string &target_file_path) const;

      protected:
        size_t index_;
        long begin_;
        long end_;
        long capacity_;
        long origin_capacity_;
        std::string file_path_;
        FILE *file_;
        CURL *curl_;
        std::shared_ptr<SliceManage> slice_manager_;
      private:
        friend size_t DownloadWriteCallback(char *buffer, size_t size, size_t nitems, void *outstream);
    };
}

#endif