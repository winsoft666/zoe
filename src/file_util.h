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
long GetFileSize(FILE* f);
utf8string GetSystemTmpDirectory();
bool CreateDirectories(const utf8string& path);
utf8string GetDirectory(const utf8string& path);
utf8string GetFileName(const utf8string& path);
utf8string AppendFileName(const utf8string& dir, const utf8string& filename);
bool FileIsExist(const utf8string& filepath);
bool FileIsRW(const utf8string& filepath);
bool RemoveFile(const utf8string& filepath);
FILE* OpenFile(const utf8string& path, const utf8string& mode);
}  // namespace teemo

#endif