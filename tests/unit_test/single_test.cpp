#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(SingleTest, test1) {
  if (http_test_datas.size() == 0)
    return;
  TestData test_data = http_test_datas[0];

  Teemo::GlobalInit();
  {
    Teemo efd;
    efd.setThreadNum(6);
    efd.setSlicePolicy(SlicePolicy::FixedNum, 10);
    if (test_data.md5.length() > 0)
      efd.setHashVerifyPolicy(ALWAYS, MD5, test_data.md5);

    efd.start(
        test_data.url, test_data.target_file_path,
        [](Result result) {
          printf("\nResult: %s\n", GetResultString(result));
          EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
        },
        [](int64_t total, int64_t downloaded) {
          if (total > 0)
            printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
        },
        nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    efd.stop();

    std::shared_future<Result> future_result = efd.futureResult();
    if (future_result.valid()) {
      Result ret = future_result.get();
      printf("\nResult: %s\n", GetResultString(ret));
    }

    future_result.wait();
  }
  Teemo::GlobalUnInit();
}
