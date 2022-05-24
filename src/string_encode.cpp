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

#include "string_encode.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "teemo/config.h"
#include <windows.h>

namespace TEEMO_NAMESPACE {
std::wstring Utf8ToUnicode(const std::string& str) {
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

std::string UnicodeToUtf8(const std::wstring& str) {
  std::string strRes;

  int iSize =
      ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);

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
}  // namespace teemo
#endif