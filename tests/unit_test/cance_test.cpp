#include "gtest/gtest.h"
#include "test_data.h"
#include "../md5.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

void DoCancelTest(std::vector<TestData> test_datas, int thread_num) {
  for (auto test_data : test_datas) {
    Teemo efd;
    efd.setThreadNum(thread_num);
    CancelEvent cancel_event;

    std::thread t = std::thread([&cancel_event]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(2500));
      cancel_event.Cancel();
    });
    t.detach();

    Result ret =
        efd.start(
               test_data.url, test_data.target_file_path,
               [test_data](Result result) {
                 printf("\nResult: %s\n", GetResultString(result));
                 EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
                 if (result == Result::SUCCESSED) {
                   if (test_data.md5.length()) {
                     EXPECT_TRUE(test_data.md5 == base::GetFileMd5(test_data.target_file_path));
                   }
                 }
               },
               [](long total, long downloaded) {
                 if (total > 0)
                   printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
               },
               nullptr, &cancel_event)
            .get();
  }
}

TEST(CancelTest, Http_ThreadNum3) {
  DoCancelTest(http_test_datas, 3);
}
