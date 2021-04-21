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

#ifndef TEEMO_FILE_UTIL_H__
#define TEEMO_FILE_UTIL_H__
#pragma once

#include <stdio.h>
#include "teemo/teemo.h"

namespace teemo {
  class FileUtil {
  public:
    static int64_t GetFileSize(FILE* f);
    static int64_t GetFileSize(const utf8string &path);
    static utf8string GetSystemTmpDirectory();
    static bool CreateDirectories(const utf8string& path);
    static utf8string GetDirectory(const utf8string& path);
    static utf8string GetFileName(const utf8string& path);
    static utf8string AppendFileName(const utf8string& dir, const utf8string& filename);
    static bool IsExist(const utf8string& filepath);
    static bool IsRW(const utf8string& filepath);
    static bool RemoveFile(const utf8string& filepath);
    static bool Rename(const utf8string& from, const utf8string &to);
    static FILE* Open(const utf8string& path, const utf8string& mode);
    static int Seek(FILE* f, int64_t offset, int origin);
    static void Close(FILE* f);
    static bool CreateFixedSizeFile(const utf8string& path, int64_t fixed_size);
    static bool PathFormatting(const utf8string& path, utf8string& formatted);
  };
}  // namespace teemo

#endif