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

#include "gtest/gtest.h"
#include "zoe/zoe.h"
#include "test_data.h"
#include <future>
using namespace zoe;

void DoTest(const std::vector<TestData>& test_datas, int thread_num) {
  Zoe::GlobalInit();

  for (const auto& test_data : test_datas) {
    Zoe efd;

    efd.setThreadNum(thread_num);
    if (test_data.md5.length() > 0)
      efd.setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, test_data.md5);

    ZoeResult ret =
        efd.start(
               test_data.url, test_data.target_file_path,
               [test_data](ZoeResult result) {
                 printf("\nResult: %s\n", Zoe::GetResultString(result));
                 EXPECT_TRUE(result == ZoeResult::SUCCESSED);
               },
               [](int64_t total, int64_t downloaded) {
                 if (total > 0)
                   printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
               },
               nullptr)
            .get();
  }

  Zoe::GlobalUnInit();
}

TEST(MultiThreadHttpTest, Http_DefaultThreadNum) {
  DoTest(http_test_datas, -1);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(MultiThreadHttpTest, Http_ThreadNum_2) {
  DoTest(http_test_datas, 2);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(MultiThreadHttpTest, Http_ThreadNum_3) {
  DoTest(http_test_datas, 3);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(MultiThreadHttpTest, Http_ThreadNum_20) {
  DoTest(http_test_datas, 20);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
