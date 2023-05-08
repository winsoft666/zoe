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

#include "file_util.h"
#include "libGet/config.h"
#include <assert.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include "string_encode.h"
#include "filesystem.hpp"

namespace LIBGET_NAMESPACE {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

int64_t FileUtil::GetFileSize(FILE* f) {
  if (!f)
    return -1;
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

utf8string FileUtil::AppendFileName(const utf8string& dir,
                                    const utf8string& filename) {
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
  return ::MoveFileExW(Utf8ToUnicode(from).c_str(), Utf8ToUnicode(to).c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH |
                           MOVEFILE_COPY_ALLOWED);
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

bool FileUtil::CreateFixedSizeFile(const utf8string& path, int64_t fixed_size) {
  utf8string str_dir = GetDirectory(path);
  if (str_dir.length() > 0 && !CreateDirectories(str_dir))
    return false;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  bool prealloc = false;
  HANDLE h = INVALID_HANDLE_VALUE;
  do {
    h = CreateFileW(Utf8ToUnicode(path).c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
      break;

    LARGE_INTEGER offset;
    offset.QuadPart = fixed_size;
    if (SetFilePointerEx(h, offset, NULL, FILE_BEGIN) == 0)
      break;

    if (!SetEndOfFile(h))
      break;

    prealloc = true;
  } while (false);

  if (h != INVALID_HANDLE_VALUE) {
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }

  if (prealloc) {
    return true;
  }

  // Candidacy
  //
  FILE* f = nullptr;
  do {
    f = Open(path, "wb+");
    if (!f)
      break;

    if (fixed_size == 0) {
      fflush(f);
      prealloc = true;
      break;
    }

    if (Seek(f, fixed_size - 1, SEEK_SET) != 0)
      break;

    if (fwrite("", 1, 1, f) != 1)
      break;
    fflush(f);

    int64_t size = GetFileSize(f);
    if (size != fixed_size)
      break;
    prealloc = true;
  } while (false);

  if (f) {
    fclose(f);
    f = nullptr;
  }

  return prealloc;
#else
  int fd = open(path.c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    return false;
  }
  if (fixed_size > 0) {
    if (fallocate(fd, 0, 0, fixed_size) != 0) {
      close(fd);
      return false;
    }
  }
  close(fd);

  return true;
#endif
}

bool FileUtil::PathFormatting(const utf8string& path, utf8string& formatted) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  utf8string cleanupPath = path;
  for (size_t i = 0; i < cleanupPath.size(); i++) {
    if (cleanupPath[i] == '/')
      cleanupPath[i] = '\\';
  }

  // See: https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
  //
  utf8string fullFileName;
  size_t lastBackslashPos = cleanupPath.find_last_of('\\');
  if (lastBackslashPos != utf8string::npos) {
    fullFileName = cleanupPath.substr(lastBackslashPos + 1);
  }
  else {
    fullFileName = cleanupPath;
  }

  if (fullFileName.length() == 0) {
    return false;
  }

  bool includeInvalidChar = false;
  for (size_t i = 0; i < fullFileName.length(); i++) {
    char c = fullFileName[i];
    if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '<' ||
        c == '>' || c == '|' || c == '"') {
      includeInvalidChar = true;
      break;
    }
  }

  if (includeInvalidChar) {
    return false;
  }

  // See: https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd
  //
  std::wstring pathW = Utf8ToUnicode(cleanupPath);
  DWORD dwRet = ::GetFullPathNameW(pathW.c_str(), 0, NULL, NULL);
  if (dwRet == 0)
    return false;

  wchar_t* pBuf = new wchar_t[dwRet + 1]();
  dwRet = ::GetFullPathNameW(pathW.c_str(), dwRet, pBuf, NULL);
  if (dwRet == 0) {
    delete[] pBuf;
    return false;
  }

  formatted = UnicodeToUtf8(pBuf);
  delete[] pBuf;

  return true;
#else
  formatted = path;
  return true;
#endif
}

}  // namespace libGet
