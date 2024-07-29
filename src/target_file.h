/*******************************************************************************
*    Copyright (C) <2019-2024>, winsoft666, <winsoft666@outlook.com>.
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

#ifndef TARGET_FILE_H__
#define TARGET_FILE_H__
#pragma once

#include "zoe/zoe.h"
#include <mutex>

namespace zoe {
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
  ZoeResult calculateFileHash(Options* opt, utf8string& str_hash);
  ZoeResult calculateFileMd5(Options* opt, utf8string& str_hash);

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
}  // namespace zoe
#endif