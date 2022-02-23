#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

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
