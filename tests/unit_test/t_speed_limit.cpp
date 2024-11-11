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

#include "catch.hpp"
#include "zoe/zoe.h"
#include "test_data.h"
#include <future>
using namespace zoe;

TEST_CASE("SpeedLimitTest") {
  if (http_test_datas.empty())
    return;

  TestData test_data = GetHttpTestData();
  printf("\nUrl: %s\n", test_data.url.c_str());

  Zoe::GlobalInit();
  {
    Zoe efd1;

    efd1.setThreadNum(3);
    efd1.setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, test_data.md5);
    efd1.setMaxDownloadSpeed(1024 * 100);
    efd1.setHttpHeaders({{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36"}});

    std::shared_future<ZoeResult> future_result1 = efd1.start(
        test_data.url,
        test_data.target_file_path,
        [](ZoeResult result) {
          printf("\nResult: %s\n", Zoe::GetResultString(result));
          REQUIRE(result == ZoeResult::SUCCESSED);
        },
        nullptr, [](int64_t byte_per_sec) { printf("%.3f kb/s\n", (float)byte_per_sec / 1024.f); });

    future_result1.wait();
  }
  Zoe::GlobalUnInit();
}