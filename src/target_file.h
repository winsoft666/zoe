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

#include "teemo/teemo.h"
#include <mutex>

namespace teemo {
typedef struct _Options Options;

class TargetFile {
 public:
  TargetFile(const utf8string& file_path);
  virtual ~TargetFile();

  bool createNew(int64_t fixed_size);
  bool open();
  void close();
  bool renameTo(Options* opt,
                const utf8string& new_file_path,
                bool need_reopen);
  Result calculateFileHash(Options* opt, utf8string& str_hash);
  Result calculateFileMd5(Options* opt, utf8string& str_hash);

  int64_t fileSize();

  int64_t write(int64_t pos, const void* data, int64_t data_size);

  utf8string filePath() const;
  int64_t fixedSize() const;
  bool isOpened() const;

 protected:
  int64_t file_seek_pos_;
  int64_t fixed_size_;

  utf8string file_path_;
  FILE* f_;
  std::recursive_mutex file_mutex_;
};
}  // namespace teemo
#endif