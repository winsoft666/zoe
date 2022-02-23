#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(SpeedTest, test1) {
  if (http_test_datas.size() == 0)
    return;
  TestData test_data = http_test_datas[0];

  Teemo::GlobalInit();
  {
    // 1
    Teemo efd1;

    efd1.setThreadNum(3);
    efd1.setHashVerifyPolicy(ALWAYS, MD5, test_data.md5);

    std::shared_future<Result> future_result1 = efd1.start(
        test_data.url, test_data.target_file_path,
        [](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
        },
        nullptr, [](int64_t byte_per_sec) { printf("EFD1: %lld\n", byte_per_sec); });

    // 2
    Teemo efd2;
    efd2.setThreadNum(3);
    efd2.setHashVerifyPolicy(ALWAYS, MD5, http_test_datas[1].md5);

    std::shared_future<Result> future_result2 = efd2.start(
        http_test_datas[1].url, http_test_datas[1].target_file_path,
        [](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
        },
        nullptr, [](int64_t byte_per_sec) { printf("EFD2: %lld\n", byte_per_sec); });

    future_result1.wait();
    future_result2.wait();
  }
  Teemo::GlobalUnInit();

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}