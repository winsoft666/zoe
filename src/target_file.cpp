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
#include "target_file.h"
#include "file_util.h"
#include <assert.h>

namespace teemo {

TargetFile::TargetFile() : f_(nullptr), file_opened_(false), fixed_size_(0L), file_seek_pos_(0L) {}

TargetFile::~TargetFile() {
  assert(f_ == nullptr && file_opened_ == false);
}

bool TargetFile::Create(const utf8string& file_path, int64_t fixed_size) {
  std::lock_guard<std::recursive_mutex> lg(file_mutex_);

  assert(f_ == nullptr && file_opened_ == false);

  if (fixed_size < 0)
    fixed_size = 0;

  if (!FileUtil::CreateFixedSizeFile(file_path, fixed_size))
    return false;

  f_ = FileUtil::OpenFile(file_path, "r+b");
  if (f_) {
    file_path_ = file_path;
    file_opened_ = true;
    file_seek_pos_ = 0L;
    fseek(f_, (long)file_seek_pos_, SEEK_SET);
  }

  return (f_ != nullptr);
}

bool TargetFile::Open(const utf8string& file_path) {
  std::lock_guard<std::recursive_mutex> lg(file_mutex_);

  if (!FileUtil::FileIsExist(file_path)) {
    return false;
  }

  assert(f_ == nullptr && file_opened_ == false);

  f_ = FileUtil::OpenFile(file_path, "r+b");
  if (f_) {
    file_path_ = file_path;
    file_opened_ = true;
    file_seek_pos_ = 0L;
    fseek(f_, (long)file_seek_pos_, SEEK_SET);
  }
  return (f_ != nullptr);
}

void TargetFile::Close() {
  std::lock_guard<std::recursive_mutex> lg(file_mutex_);
  if (f_) {
    fflush(f_);
    fclose(f_);
    f_ = nullptr;
    file_opened_ = false;
  }
}

int64_t TargetFile::Write(int64_t pos, const void* data, int64_t data_size) {
  std::lock_guard<std::recursive_mutex> lg(file_mutex_);
  assert(f_ && file_opened_);
  int64_t written = 0L;
  do {
    if (!f_)
      break;
    if (!data || data_size == 0)
      break;
    if (pos < 0)
      break;
    if (file_seek_pos_ != pos) {
      if (fseek(f_, (long)pos, SEEK_SET) != 0) {
        break;
      }
      file_seek_pos_ = pos;
    }

    written = fwrite(data, 1, (long)data_size, f_);
    fflush(f_);
    file_seek_pos_ += written;
  } while (false);

  return written;
}

utf8string TargetFile::filePath() const {
  return file_path_;
}

int64_t TargetFile::fixedSize() const {
  return fixed_size_;
}

bool TargetFile::IsOpened() const {
  return file_opened_;
}

}  // namespace teemo