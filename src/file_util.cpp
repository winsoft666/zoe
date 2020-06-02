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
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef GHC_USE_STD_FS
#include "filesystem.hpp"
#endif

namespace teemo {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PATH_SEPARATOR '\\'

static std::wstring Utf8ToUnicode(const std::string& str) {
  std::wstring strRes;
  int iSize = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

  if (iSize == 0)
    return strRes;

  wchar_t* szBuf = new (std::nothrow) wchar_t[iSize];

  if (!szBuf)
    return strRes;

  memset(szBuf, 0, iSize * sizeof(wchar_t));
  ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, szBuf, iSize);

  strRes = szBuf;
  delete[] szBuf;

  return strRes;
}

static std::string UnicodeToUtf8(const std::wstring& str) {
  std::string strRes;

  int iSize = ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);

  if (iSize == 0)
    return strRes;

  char* szBuf = new (std::nothrow) char[iSize];

  if (!szBuf)
    return strRes;

  memset(szBuf, 0, iSize);

  ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, szBuf, iSize, NULL, NULL);

  strRes = szBuf;
  delete[] szBuf;

  return strRes;
}
#else
#define PATH_SEPARATOR '/'
#endif

int64_t FileUtil::GetFileSize(FILE* f) {
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  int64_t fsize = ftell(f);
  return fsize;
}

int64_t FileUtil::GetFileSize(const utf8string& path) {
  int64_t fsize = -1;
  FILE* f = OpenFile(path, "rb");
  if (f) {
    fsize = GetFileSize(f);
    fclose(f);
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
    if (!FileIsExist(target_dir)) {
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

bool FileUtil::FileIsExist(const utf8string& filepath) {
  if (filepath.length() == 0)
    return false;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_waccess(unicode_filepath.c_str(), 0) == 0);
#else
  return (access(filepath.c_str(), F_OK) == 0);
#endif
}

bool FileUtil::FileIsRW(const utf8string& filepath) {
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

bool FileUtil::RenameFile(const utf8string& from,
                                const utf8string& to,
                                bool allow_remove_old) {
  if (FileIsExist(to)) {
    if (allow_remove_old && !RemoveFile(to)) {
      return false;
    }
  }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_from = Utf8ToUnicode(from);
  std::wstring unicode_to = Utf8ToUnicode(to);

  return (_wrename(unicode_from.c_str(), unicode_to.c_str()) == 0);
#else
  return (rename(from.c_str(), to.c_str()) == 0);
#endif
}

FILE* FileUtil::OpenFile(const utf8string& path, const utf8string& mode) {
  FILE* f = nullptr;
  if (path.length() == 0 || mode.length() == 0)
    return f;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_path = Utf8ToUnicode(path);
  std::wstring unicode_mode = Utf8ToUnicode(mode);
  f = _wfopen(unicode_path.c_str(), unicode_mode.c_str());
#else
  f = fopen(path.c_str(), mode.c_str());
#endif
  return f;
}

bool FileUtil::CreateFixedSizeFile(const utf8string& path, int64_t fixed_size) {
  utf8string str_dir = GetDirectory(path);
  if (str_dir.length() > 0 && !CreateDirectories(str_dir))
    return false;

  FILE* f = OpenFile(path, "wb");
  if (!f)
    return false;
  if (fixed_size == 0) {
    fflush(f);
    fclose(f);
    return true;
  }

  if (fseek(f, (long)(fixed_size - 1), SEEK_SET) != 0) {
    fflush(f);
    fclose(f);
    return false;
  }
  if (fwrite("", 1, 1, f) != 1) {
    fflush(f);
    fclose(f);
    return false;
  }
  fflush(f);
  fclose(f);
  f = nullptr;

  // check
  f = OpenFile(path, "rb");
  if (!f)
    return false;
  int64_t size = GetFileSize(f);
  fflush(f);
  fclose(f);
  return (fixed_size == size);
}

}  // namespace teemo
