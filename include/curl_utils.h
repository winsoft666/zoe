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

#ifndef TEEMO_GLOBAL_ENV_H_
#define TEEMO_GLOBAL_ENV_H_
#pragma once

#include "curl/curl.h"

namespace teemo {
void GlobalCurlInit();
void GlobalCurlUnInit();

class ScopedCurl {
public:
  ScopedCurl() { curl_ = curl_easy_init(); }

  ~ScopedCurl() { curl_easy_cleanup(curl_); }

  CURL *GetCurl() { return curl_; }

private:
  CURL *curl_;
};
} // namespace teemo

#endif // !TEEMO_GLOBAL_ENV_H_