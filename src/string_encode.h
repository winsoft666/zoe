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