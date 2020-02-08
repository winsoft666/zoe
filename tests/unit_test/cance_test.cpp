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
    efd.SetThreadNum(thread_num);
    efd.SetSaveSliceFileToTempDir(true);
    Concurrency::cancellation_token_source cts;

    std::thread t = std::thread([cts]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(2500));
      cts.cancel();
    });
    t.detach();

    efd.Start(
           test_data.url, test_data.target_file_path,
           [](long total, long downloaded) {
             if (total > 0)
               printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
           },
           nullptr, cts)
        .then([=](pplx::task<Result> result) {
          printf("\nResult: %s\n", GetResultString(result.get()));
          EXPECT_TRUE(result.get() == Successed || result.get() == Canceled);
          if (result.get() == Result::Successed) {
            if (test_data.md5.length()) {
              EXPECT_TRUE(test_data.md5 == base::GetFileMd5(test_data.target_file_path));
            }
          }
        })
        .wait();
  }
}

TEST(CancelTest, Http_ThreadNum3) {
  DoCancelTest(http_test_datas, 3);
}
