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

#ifndef ZOE_STRING_HELPER_H_
#define ZOE_STRING_HELPER_H_
#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cwctype>
#include <iterator>

namespace zoe {
class StringHelper {
 public:
  static char EasyCharToLowerA(char in) {
    if (in <= 'Z' && in >= 'A')
      return in - ('Z' - 'z');
    return in;
  }

  static char EasyCharToUpperA(char in) {
    if (in <= 'z' && in >= 'a')
      return in + ('Z' - 'z');
    return in;
  }

  static wchar_t EasyCharToLowerW(wchar_t in) {
    if (in <= 'Z' && in >= 'A')
      return in - (L'Z' - L'z');
    return in;
  }

  static wchar_t EasyCharToUpperW(wchar_t in) {
    if (in <= L'z' && in >= L'a')
      return in + (L'Z' - L'z');
    return in;
  }

  static std::string ToLower(const std::string& s) {
    std::string d = s;
    std::transform(d.begin(), d.end(), d.begin(), EasyCharToLowerA);
    return d;
  }

  static std::wstring ToLower(const std::wstring& s) {
    std::wstring d = s;
    std::transform(d.begin(), d.end(), d.begin(), EasyCharToLowerW);
    return d;
  }

  static std::string ToUpper(const std::string& s) {
    std::string d = s;
    std::transform(d.begin(), d.end(), d.begin(), EasyCharToUpperA);
    return d;
  }

  static std::wstring ToUpper(const std::wstring& s) {
    std::wstring d = s;
    std::transform(d.begin(), d.end(), d.begin(), EasyCharToUpperW);
    return d;
  }

  static std::string Trim(const std::string& s, char c = ' ') {
    const std::string::size_type pos = s.find_first_not_of(c);
    if (pos == std::string::npos) {
      return std::string();
    }

    std::string::size_type n = s.find_last_not_of(c) - pos + 1;
    return s.substr(pos, n);
  }

  static std::wstring Trim(const std::wstring& s, wchar_t c = L' ') {
    const std::wstring::size_type pos = s.find_first_not_of(c);
    if (pos == std::wstring::npos) {
      return std::wstring();
    }

    std::wstring::size_type n = s.find_last_not_of(c) - pos + 1;
    return s.substr(pos, n);
  }

  static std::string LeftTrim(const std::string& s, char c = ' ') {
    const std::string::size_type pos = s.find_first_not_of(c);
    if (pos == std::string::npos) {
      return std::string();
    }

    return s.substr(pos);
  }

  static std::wstring LeftTrim(const std::wstring& s, wchar_t c = L' ') {
    const std::wstring::size_type pos = s.find_first_not_of(c);
    if (pos == std::wstring::npos) {
      return std::wstring();
    }

    return s.substr(pos);
  }

  static std::string RightTrim(const std::string& s, char c = ' ') {
    const std::string::size_type pos = s.find_last_not_of(c);
    if (pos == 0) {
      return std::string();
    }

    return s.substr(0, pos + 1);
  }

  static std::wstring RightTrim(const std::wstring& s, wchar_t c = L' ') {
    const std::wstring::size_type pos = s.find_last_not_of(c);
    if (pos == 0) {
      return std::wstring();
    }

    return s.substr(0, pos + 1);
  }

  static bool IsStartsWith(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.length(), prefix) == 0;
  }

  static bool IsStartsWith(const std::wstring& s, const std::wstring& prefix) {
    return s.compare(0, prefix.length(), prefix) == 0;
  }

  static bool IsEndsWith(const std::string& s, const std::string& suffix) {
    if (suffix.length() <= s.length()) {
      return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    return false;
  }

  static bool IsEndsWith(const std::wstring& s, const std::wstring& suffix) {
    if (suffix.length() <= s.length()) {
      return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    return false;
  }

  static bool IsContains(const std::string& str, const std::string& substring) {
    return (str.find(substring) != std::string::npos);
  }

  static bool IsContains(const std::wstring& str, const std::wstring& substring) {
    return (str.find(substring) != std::wstring::npos);
  }

  static size_t ContainTimes(const std::string& str, const std::string& substring) {
    size_t times = 0;
    size_t pos = std::string::npos;
    size_t offset = 0;

    if (substring.length() == 0)
      return 0;

    do {
      pos = str.find(substring, offset);
      if (pos == std::string::npos)
        break;

      offset = pos + substring.length();
      times++;
    } while (true);

    return times;
  }

  static size_t ContainTimes(const std::wstring& str, const std::wstring& substring) {
    size_t times = 0;
    size_t pos = std::wstring::npos;
    size_t offset = 0;

    if (substring.length() == 0)
      return 0;

    do {
      pos = str.find(substring, offset);
      if (pos == std::wstring::npos)
        break;

      offset = pos + substring.length();
      times++;
    } while (true);

    return times;
  }

  static std::string ReplaceFirst(const std::string& s, const std::string& from, const std::string& to) {
    const size_t start_pos = s.find(from);
    if (start_pos == std::string::npos) {
      return s;
    }

    std::string ret = s;
    ret.replace(start_pos, from.length(), to);
    return ret;
  }

  static std::wstring ReplaceFirst(const std::wstring& s, const std::wstring& from, const std::wstring& to) {
    const size_t start_pos = s.find(from);
    if (start_pos == std::wstring::npos) {
      return s;
    }

    std::wstring ret = s;
    ret.replace(start_pos, from.length(), to);
    return ret;
  }

  static std::string ReplaceLast(const std::string& s, const std::string& from, const std::string& to) {
    const size_t start_pos = s.rfind(from);
    if (start_pos == std::string::npos) {
      return s;
    }

    std::string ret = s;
    ret.replace(start_pos, from.length(), to);
    return ret;
  }

  static std::wstring ReplaceLast(const std::wstring& s, const std::wstring& from, const std::wstring& to) {
    const size_t start_pos = s.rfind(from);
    if (start_pos == std::wstring::npos) {
      return s;
    }

    std::wstring ret = s;
    ret.replace(start_pos, from.length(), to);
    return ret;
  }

  static std::string Replace(const std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) {
      return s;
    }

    size_t start_pos = 0;
    const bool found_substring = s.find(from, start_pos) != std::string::npos;
    if (!found_substring) {
      return s;
    }

    std::string ret = s;
    while ((start_pos = ret.find(from, start_pos)) != std::string::npos) {
      ret.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }

    return ret;
  }

  static std::wstring Replace(const std::wstring& s, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) {
      return s;
    }

    size_t start_pos = 0;
    const bool found_substring = s.find(from, start_pos) != std::string::npos;
    if (!found_substring) {
      return s;
    }

    std::wstring ret = s;
    while ((start_pos = ret.find(from, start_pos)) != std::string::npos) {
      ret.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }

    return ret;
  }

  static std::vector<std::string> Split(const std::string& src, const std::string& delimiter, bool includeEmptyStr = true) {
    std::vector<std::string> fields;
    typename std::string::size_type offset = 0;
    typename std::string::size_type pos = src.find(delimiter, 0);

    while (pos != std::string::npos) {
      std::string t = src.substr(offset, pos - offset);
      if ((t.length() > 0) || (t.length() == 0 && includeEmptyStr))
        fields.push_back(t);
      offset = pos + delimiter.length();
      pos = src.find(delimiter, offset);
    }

    const std::string t = src.substr(offset);
    if ((t.length() > 0) || (t.length() == 0 && includeEmptyStr))
      fields.push_back(t);
    return fields;
  }

  static std::vector<std::wstring> Split(const std::wstring& src, const std::wstring& delimiter, bool includeEmptyStr = true) {
    std::vector<std::wstring> fields;
    typename std::wstring::size_type offset = 0;
    typename std::wstring::size_type pos = src.find(delimiter, 0);

    while (pos != std::wstring::npos) {
      std::wstring t = src.substr(offset, pos - offset);
      if ((t.length() > 0) || (t.length() == 0 && includeEmptyStr))
        fields.push_back(t);
      offset = pos + delimiter.length();
      pos = src.find(delimiter, offset);
    }

    const std::wstring t = src.substr(offset);
    if ((t.length() > 0) || (t.length() == 0 && includeEmptyStr))
      fields.push_back(t);
    return fields;
  }

  static std::string Join(const std::vector<std::string>& src, const std::string& delimiter, bool includeEmptyStr = true) {
    std::stringstream ss;
    for (std::vector<std::string>::const_iterator it = src.cbegin(); it != src.cend(); ++it) {
      if (it->length() > 0) {
        ss << *it;
      }
      else {
        if (includeEmptyStr) {
          ss << *it;
        }
      }

      if (it + 1 != src.cend()) {
        ss << delimiter;
      }
    }
    return ss.str();
  }

  static std::wstring Join(const std::vector<std::wstring>& src, const std::wstring& delimiter, bool includeEmptyStr = true) {
    std::wstringstream ss;
    for (std::vector<std::wstring>::const_iterator it = src.cbegin(); it != src.cend(); ++it) {
      if (it->length() > 0) {
        ss << *it;
      }
      else {
        if (includeEmptyStr) {
          ss << *it;
        }
      }

      if (it + 1 != src.cend()) {
        ss << delimiter;
      }
    }
    return ss.str();
  }

  static bool IsEqual(const std::string& s1, const std::string& s2, bool ignoreCase = false) {
    const std::string::size_type s1_len = s1.length();
    if (s1_len != s2.length())
      return false;

    for (std::string::size_type i = 0; i < s1_len; i++) {
      if (ignoreCase) {
        if (EasyCharToLowerA(s1[i]) != EasyCharToLowerA(s2[i]))
          return false;
      }
      else {
        if (s1[i] != s2[i])
          return false;
      }
    }

    return true;
  }

  static bool IsEqual(const std::wstring& s1, const std::wstring& s2, bool ignoreCase = false) {
    const std::wstring::size_type s1_len = s1.length();
    if (s1_len != s2.length())
      return false;

    for (std::wstring::size_type i = 0; i < s1_len; i++) {
      if (ignoreCase) {
        if (EasyCharToLowerW(s1[i]) != EasyCharToLowerW(s2[i]))
          return false;
      }
      else {
        if (s1[i] != s2[i])
          return false;
      }
    }

    return true;
  }
};

}  // namespace zoe

#endif