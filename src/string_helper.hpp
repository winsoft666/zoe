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

#ifndef TEEMO_STRING_HELPER_H_
#define TEEMO_STRING_HELPER_H_
#pragma once
#include "teemo/teemo.h"

namespace teemo {
inline char EasyCharToLowerA(char in) {
  if (in <= 'Z' && in >= 'A')
    return in - ('Z' - 'z');
  return in;
}

inline char EasyCharToUpperA(char in) {
  if (in <= 'z' && in >= 'a')
    return in + ('Z' - 'z');
  return in;
}

inline wchar_t EasyCharToLowerW(wchar_t in) {
  if (in <= 'Z' && in >= 'A')
    return in - (L'Z' - L'z');
  return in;
}

inline wchar_t EasyCharToUpperW(wchar_t in) {
  if (in <= L'z' && in >= L'a')
    return in + (L'Z' - L'z');
  return in;
}

template <typename T, typename Func>
typename std::enable_if<std::is_same<char, T>::value || std::is_same<wchar_t, T>::value,
                        std::basic_string<T, std::char_traits<T>, std::allocator<T>>>::type
StringCaseConvert(const std::basic_string<T, std::char_traits<T>, std::allocator<T>>& str,
                  Func func) {
  std::basic_string<T, std::char_traits<T>, std::allocator<T>> ret = str;
  std::transform(ret.begin(), ret.end(), ret.begin(), func);
  return ret;
}
}  // namespace teemo

#endif