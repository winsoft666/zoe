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

#include <future>
#include "catch.hpp"
#include "zoe/zoe.h"
#include "test_data.h"
using namespace zoe;

void DoTest(const TestData& test_data, int thread_num, int32_t disk_cache) {
  printf("\nUrl: %s\n", test_data.url.c_str());

  Zoe::GlobalInit();

  Zoe z;

  if (thread_num != -1)
    z.setThreadNum(thread_num);

  z.setDiskCacheSize(disk_cache);
  if (test_data.md5.length() > 0)
    z.setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, test_data.md5);

  z.setHttpHeaders({{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36"}});

  ZoeResult ret =
      z.start(
           test_data.url, test_data.target_file_path,
           [test_data](ZoeResult result) {
             printf("\nResult: %s\n", Zoe::GetResultString(result));
             CHECK((result == ZoeResult::SUCCESSED || result == ZoeResult::CANCELED));
           },
           [](int64_t total, int64_t downloaded) {
             if (total > 0)
               printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
           },
           nullptr)
          .get();

  Zoe::GlobalUnInit();
}

TEST_CASE("DiskCacheHttpTest-ThreadNum10-DiskCache10MB") {
  DoTest(GetHttpTestData(), 10, 10 * 1024 * 1024);
}

TEST_CASE("DiskCacheHttpTest-ThreadNum3-DiskCache0") {
  DoTest(GetHttpTestData(), 3, 0);
}
