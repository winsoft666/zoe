#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(SpeedTest, test1) {
  Teemo::GlobalInit();
  {
    // 1
    Teemo efd1;

    efd1.setThreadNum(3);
    efd1.setHashVerifyPolicy(ALWAYS, MD5, http_test_datas[0].md5);

    efd1.start(
        http_test_datas[0].url, http_test_datas[0].target_file_path,
        [](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
        },
        nullptr, [](int64_t byte_per_sec) { printf("EFD1: %lld\n", byte_per_sec); });

    // 2
    Teemo efd2;
    efd2.setThreadNum(3);
    efd2.setHashVerifyPolicy(ALWAYS, MD5, http_test_datas[1].md5);

    efd2.start(
        http_test_datas[1].url, http_test_datas[1].target_file_path,
        [](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
        },
        nullptr, [](int64_t byte_per_sec) { printf("EFD2: %lld\n", byte_per_sec); });

    getchar();
  }
  Teemo::GlobalUnInit();
}