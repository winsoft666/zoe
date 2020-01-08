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

namespace teemo {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PATH_SEPARATOR '\\'

static std::wstring Utf8ToUnicode(const std::string &str) {
  std::wstring strRes;
  int iSize = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

  if (iSize == 0)
    return strRes;

  wchar_t *szBuf = new (std::nothrow) wchar_t[iSize];

  if (!szBuf)
    return strRes;

  memset(szBuf, 0, iSize * sizeof(wchar_t));
  ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, szBuf, iSize);

  strRes = szBuf;
  delete[] szBuf;

  return strRes;
}
#else
#define PATH_SEPARATOR '/'
#endif

long GetFileSize(FILE *f) {
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  return fsize;
}

utf8string GetDirectory(const utf8string &path) {
  utf8string::size_type pos = path.find_last_of(PATH_SEPARATOR);
  return path.substr(0, pos);
}

utf8string GetFileName(const utf8string &path) {
  utf8string::size_type pos = path.find_last_of(PATH_SEPARATOR);
  if (pos == utf8string::npos)
    pos = 0;
  else
    pos++;
  return path.substr(pos);
}

utf8string AppendFileName(const utf8string &dir, const utf8string &filename) {
  utf8string result = dir;
  if (result.length() > 0) {
    if (result[result.length() - 1] != PATH_SEPARATOR)
      result += PATH_SEPARATOR;
  }

  result += filename;
  return result;
}

bool FileIsExist(const utf8string &filepath) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_waccess(unicode_filepath.c_str(), 0) == 0);
#else
  return (access(filepath.c_str(), F_OK) == 0);
#endif
}

bool FileIsRW(const utf8string &filepath) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_waccess(unicode_filepath.c_str(), 6) == 0);
#else
  return (access(filepath.c_str(), R_OK | W_OK) == 0);
#endif
}

bool RemoveFile(const utf8string &filepath) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_filepath = Utf8ToUnicode(filepath);
  return (_wremove(unicode_filepath.c_str()) == 0);
#else
  return (remove(filepath.c_str()) != 0);
#endif
}

FILE *OpenFile(const utf8string &path, const utf8string &mode) {
  FILE *f = nullptr;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::wstring unicode_path = Utf8ToUnicode(path);
  std::wstring unicode_mode = Utf8ToUnicode(mode);
  f = _wfopen(unicode_path.c_str(), unicode_mode.c_str());
#else
  f = fopen(path.c_str(), mode.c_str());
#endif
  return f;
}

} // namespace teemo
