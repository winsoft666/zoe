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

void DoBreakpointTest(const std::vector<TestData>& test_datas, int thread_num) {
  for (const auto &test_data : test_datas) {
    std::future<void> test_task = std::async(std::launch::async, [test_data, thread_num]() {
      Zoe efd;

      efd.setThreadNum(thread_num / 2);
      if (test_data.md5.length() > 0)
        efd.setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, test_data.md5);

      std::shared_future<ZoeResult> r = efd.start(
          test_data.url, test_data.target_file_path,
          [test_data](ZoeResult result) {
            printf("\nResult: %s\n", Zoe::GetResultString(result));
            EXPECT_TRUE(result == ZoeResult::SUCCESSED || result == ZoeResult::CANCELED);
          },
          [](int64_t total, int64_t downloaded) {
            if (total > 0)
              printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
          },
          nullptr);

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      efd.stop();

      r.wait();

      efd.setThreadNum(thread_num);

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
    });
    test_task.wait();
  }
}

TEST(BreakPointHttpTest, Http_ThreadNum_1_Breakpoint) {
  DoBreakpointTest(http_test_datas, 1);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(BreakPointHttpTest, Http_ThreadNum_3_Breakpoint) {
  DoBreakpointTest(http_test_datas, 3);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(BreakPointHttpTest, Http_ThreadNum_10_Breakpoint) {
  DoBreakpointTest(http_test_datas, 10);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
