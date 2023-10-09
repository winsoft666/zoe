/*******************************************************************************
*    Copyright (C) <2019-2023>, winsoft666, <winsoft666@outlook.com>.
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

#ifndef ZOE_GLOBAL_ENV_H_
#define ZOE_GLOBAL_ENV_H_
#pragma once

#include "curl/curl.h"

namespace zoe {
void GlobalCurlInit();
void GlobalCurlUnInit();

class ScopedCurl {
 public:
  ScopedCurl() { curl_ = curl_easy_init(); }

  ~ScopedCurl() { curl_easy_cleanup(curl_); }

  CURL* GetCurl() { return curl_; }

 private:
  CURL* curl_;
};
}  // namespace zoe

#endif  // !ZOE_GLOBAL_ENV_H_