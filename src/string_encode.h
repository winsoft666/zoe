#ifndef TEEMO_STRING_ENCODE_H_
#define TEEMO_STRING_ENCODE_H_
#pragma once

#include <string>

namespace teemo {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
std::wstring Utf8ToUnicode(const std::string& str);
std::string UnicodeToUtf8(const std::wstring& str);
#endif
}  // namespace teemo

#endif