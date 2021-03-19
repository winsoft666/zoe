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

#include "file_util.h"
#include <assert.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "string_encode.h"
#include "filesystem.hpp"

namespace teemo {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

int64_t FileUtil::GetFileSize(FILE* f) {
  if (!f)
    return 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  _fseeki64(f, 0L, SEEK_END);
  int64_t fsize = _ftelli64(f);
#else
  fseeko64(f, 0L, SEEK_END);
  int64_t fsize = ftello64(f);
#endif

  return fsize;
}

int64_t FileUtil::GetFileSize(const utf8string& path) {
  int64_t fsize = -1;
  FILE* f = Open(path, "rb");
  if (f) {
    fsize = GetFileSize(f);
    Close(f);
    f = nullptr;
  }
  return fsize;
}

utf8string FileUtil::GetSystemTmpDirectory() {
  utf8string target_dir;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  wchar_t buf[MAX_PATH] = {0};
  DWORD ret_val = GetTempPathW(MAX_PATH, buf);
  if (ret_val > 0 && ret_val < MAX_PATH) {
    target_dir = UnicodeToUtf8(buf);
  }

  if (target_dir.length() > 0) {
    if (!IsExist(target_dir)) {
      target_dir.clear();
    }
  }
#else
  target_dir = "/var/tmp/";
#endif

  return target_dir;
}

bool FileUtil::CreateDirectories(const utf8string& path) {
  if (path.length() == 0)
    return true;
  std::error_code ec;
  return ghc::filesystem::create_directories(path, ec);
}

utf8string FileUtil::GetDirectory(const utf8string& path) {
  utf8string::size_type pos = path.find_last_of(PATH_SEPARATOR);
  if (pos == utf8string::npos)
    return "";
  return path.substr(0, pos);
}

utf8string FileUtil::GetFileName(const utf8string& path) {
  utf8string::size_type pos = path.find_last_of(PATH_SEPARATOR);
  if (pos == utf8string::npos)
    pos = 0;
  else
    pos++;
  return path.substr(pos);
}

utf8string FileUtil::AppendFileName(const utf8string& dir, const utf8string& filename) {
  utf8string result = dir;
  if (result.length() > 0) {
    if (result[result.length() - 1] != PATH_SEPARATOR)
      result += PATH_SEPARATOR;
  }

  result += filename;
  return result;
}

bool FileUtil::IsExist(const utf8string& filepath) {
  if (filepath.length() == 0)
    return false;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_waccess(unicode_filepath.c_str(), 0) == 0);
#else
  return (access(filepath.c_str(), F_OK) == 0);
#endif
}

bool FileUtil::IsRW(const utf8string& filepath) {
  if (filepath.length() == 0)
    return false;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_waccess(unicode_filepath.c_str(), 6) == 0);
#else
  return (access(filepath.c_str(), R_OK | W_OK) == 0);
#endif
}

bool FileUtil::RemoveFile(const utf8string& filepath) {
  if (filepath.length() == 0)
    return false;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_wremove(unicode_filepath.c_str()) == 0);
#else
  return (remove(filepath.c_str()) != 0);
#endif
}

bool FileUtil::Rename(const utf8string& from, const utf8string& to) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  return ::MoveFileExW(Utf8ToUnicode(from).c_str(), Utf8ToUnicode(to).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED);
#else
  return 0 == rename(from.c_str(), to.c_str());
#endif
}

FILE* FileUtil::Open(const utf8string& path, const utf8string& mode) {
  FILE* f = nullptr;
  if (path.length() == 0 || mode.length() == 0)
    return f;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  _wfopen_s(&f, Utf8ToUnicode(path).c_str(), Utf8ToUnicode(mode).c_str());
#else
  f = fopen(path.c_str(), mode.c_str());
#endif

  return f;
}

int FileUtil::Seek(FILE* f, int64_t offset, int origin) {
  if (f) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    return _fseeki64(f, offset, origin);
#else
    return fseeko64(f, offset, origin);
#endif
  }
  return -1;
}

void FileUtil::Close(FILE* f) {
  if (f) {
    int err = fclose(f);
    assert(err == 0);
  }
}

FILE* FileUtil::CreateFixedSizeFile(const utf8string& path, int64_t fixed_size) {
  utf8string str_dir = GetDirectory(path);
  if (str_dir.length() > 0 && !CreateDirectories(str_dir))
    return nullptr;

  FILE* f = Open(path, "wb+");
  if (!f)
    return nullptr;

  if (fixed_size == 0) {
    fflush(f);
    Close(f);
    return nullptr;
  }

  if (Seek(f, fixed_size - 1, SEEK_SET) != 0) {
    Close(f);
    return nullptr;
  }

  if (fwrite("", 1, 1, f) != 1) {
    Close(f);
    return nullptr;
  }
  fflush(f);

  int64_t size = GetFileSize(f);
  if (size != fixed_size) {
    Close(f);
    return nullptr;
  }
  if (Seek(f, 0L, SEEK_SET) != 0) {
    Close(f);
    return nullptr;
  }

  return f;
}

}  // namespace teemo
