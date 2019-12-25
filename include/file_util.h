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

#ifndef EFD_FILE_UTIL_H__
#define EFD_FILE_UTIL_H__
#pragma once

#include <stdio.h>
#include <string>

namespace easy_file_download {
long GetFileSize(FILE *f);
std::string GetDirectory(const std::string &path);
std::string GetFileName(const std::string &path);
std::string AppendFileName(const std::string &dir, const std::string &filename);
bool FileIsExist(const std::string &filepath);
bool FileIsRW(const std::string &filepath);
} // namespace easy_file_download

#endif