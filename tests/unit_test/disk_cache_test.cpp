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

#include <future>
#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
using namespace TEEMO_NAMESPACE;

void DoTest(const std::vector<TestData>& test_datas, int thread_num, int32_t disk_cache) {
  TEEMO::GlobalInit();

  for (const auto& test_data : test_datas) {
    TEEMO efd;

    if (thread_num != -1)
      efd.setThreadNum(thread_num);

    efd.setDiskCacheSize(disk_cache);
    if (test_data.md5.length() > 0)
      efd.setHashVerifyPolicy(ALWAYS, MD5, test_data.md5);

    Result ret =
        efd.start(
               test_data.url, test_data.target_file_path,
               [test_data](Result result) {
                 printf("\nResult: %s\n", GetResultString(result));
                 EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
               },
               [](int64_t total, int64_t downloaded) {
                 if (total > 0)
                   printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
               },
               nullptr)
            .get();
  }

  TEEMO::GlobalUnInit();
}

TEST(DiskCacheHttpTest, Http_ThreadNum_2) {
  DoTest(http_test_datas, 10, 10 * 1024 * 1024);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(DiskCacheHttpTest, Http_ThreadNum_3) {
  DoTest(http_test_datas, 3, 0);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(DiskCacheHttpTest, Http_ThreadNum_20) {
  DoTest(http_test_datas, 20, 1);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
