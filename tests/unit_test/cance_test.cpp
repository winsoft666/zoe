#include "gtest/gtest.h"
#include "test_data.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

void DoCancelTest(const std::vector<TestData>& test_datas, int thread_num) {
  for (const auto& test_data : test_datas) {
    Event cancel_event;

    Teemo efd;
    efd.setThreadNum(thread_num);
    efd.setStopEvent(&cancel_event);
    if (test_data.md5.length() > 0)
      efd.setHashVerifyPolicy(ALWAYS, MD5, test_data.md5);

    std::thread t = std::thread([&cancel_event]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      cancel_event.set();
    });

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

    t.join();
  }
}

TEST(CancelTest, Http_ThreadNum3) {
  DoCancelTest(http_test_datas, 3);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
