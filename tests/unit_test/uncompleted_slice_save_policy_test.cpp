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

#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace TEEMO_NAMESPACE;

void DoTest(const std::vector<TestData>& test_datas,
            int thread_num,
            UncompletedSliceSavePolicy policy) {
  Teemo::GlobalInit();

  for (const auto& test_data : test_datas) {
    Teemo efd;

    efd.setThreadNum(thread_num);
    if (test_data.md5.length() > 0)
      efd.setHashVerifyPolicy(ALWAYS, MD5, test_data.md5);
    efd.setUncompletedSliceSavePolicy(policy);

    std::shared_future<Result> future_result = efd.start(
        test_data.url, test_data.target_file_path,
        [test_data](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
        },
        [](int64_t total, int64_t downloaded) {
          if (total > 0)
            printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
        },
        nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    efd.stop();

    future_result.wait();

    efd.start(
           test_data.url, test_data.target_file_path,
           [test_data](Result result) {
             printf("\nResult: %s\n", GetResultString(result));
             EXPECT_TRUE(result == SUCCESSED);
           },
           [](int64_t total, int64_t downloaded) {
             if (total > 0)
               printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
           },
           nullptr)
        .wait();
  }

  Teemo::GlobalUnInit();
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_DefaultThreadNum_ALWAYS_DISCARD) {
  DoTest(http_test_datas, -1, ALWAYS_DISCARD);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_DefaultThreadNum_SAVE_EXCEPT_FAILED) {
  DoTest(http_test_datas, -1, SAVE_EXCEPT_FAILED);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_ThreadNum_2_ALWAYS_DISCARD) {
  DoTest(http_test_datas, 2, ALWAYS_DISCARD);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_ThreadNum_2_SAVE_EXCEPT_FAILED) {
  DoTest(http_test_datas, 2, SAVE_EXCEPT_FAILED);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_ThreadNum_3_ALWAYS_DISCARD) {
  DoTest(http_test_datas, 3, ALWAYS_DISCARD);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_ThreadNum_3_SAVE_EXCEPT_FAILED) {
  DoTest(http_test_datas, 3, SAVE_EXCEPT_FAILED);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_ThreadNum_20_SAVE_EXCEPT_FAILED) {
  DoTest(http_test_datas, 20, SAVE_EXCEPT_FAILED);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(UncompletedSliceSavePolicyHttpTest, Http_ThreadNum_20_ALWAYS_DISCARD) {
  DoTest(http_test_datas, 20, ALWAYS_DISCARD);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
