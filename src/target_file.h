/*******************************************************************************
* Copyright (C) 2018 - 2020, winsoft666, <winsoft666@outlook.com>.
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
#ifndef TARGET_FILE_H__
#define TARGET_FILE_H__
#pragma once

#include <string>
#include "teemo/teemo.h"
#include <mutex>

namespace teemo {
  class TargetFile {
  public:
    TargetFile();
    virtual ~TargetFile();

    bool Create(const utf8string& file_path, long fixed_size);
    bool Open(const utf8string& file_path);
    void Close();

    long Write(long pos, const void* data, long data_size);


    utf8string filePath() const;
    long fixedSize() const;
    bool IsOpened() const;

  protected:
    bool file_opened_;
    long file_seek_pos_;
    long fixed_size_;

    utf8string file_path_;
    FILE* f_;
    std::recursive_mutex file_mutex_;
  };
}
#endif