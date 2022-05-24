/*******************************************************************************
*    Copyright (C) <2019-2022>, winsoft666, <winsoft666@outlook.com>.
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef TEEMO_FILE_UTIL_H__
#define TEEMO_FILE_UTIL_H__
#pragma once

#include <stdio.h>
#include "teemo/teemo.h"

namespace TEEMO_NAMESPACE {
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