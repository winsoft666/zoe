#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(SpeedLimitTest, test1) {
  if (http_test_datas.empty())
    return;

  Teemo::GlobalInit();
  {
    Teemo efd1;

    efd1.setThreadNum(3);
    efd1.setHashVerifyPolicy(ALWAYS, MD5, http_test_datas[0].md5);
    efd1.setMaxDownloadSpeed(1024 * 100);

    std::shared_future<Result> future_result1 = efd1.start(
        http_test_datas[0].url, http_test_datas[0].target_file_path,
        [](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED);
        },
        nullptr, [](int64_t byte_per_sec) { printf("%.3f kb/s\n", (float)byte_per_sec / 1024.f); });

    future_result1.wait();
  }
  Teemo::GlobalUnInit();

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}