#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace teemo;

void DoBreakpointTest(std::vector<TestData> test_datas, int thread_num) {
  for (auto test_data : test_datas) {
    std::future<void> test_task = std::async(std::launch::async, [test_data, thread_num]() {
      Teemo efd;

      efd.setThreadNum(thread_num);

      std::shared_future<Result> r = efd.start(
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
          [](int64_t total, int64_t downloaded) {
            if (total > 0)
              printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
          },
          nullptr);

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      efd.stop(true);

      r.get();
      efd.setThreadNum(thread_num);

      Result ret =
          efd.start(
                 test_data.url, test_data.target_file_path,
                 [test_data](Result result) {
                   printf("\nResult: %s\n", GetResultString(result));
                   EXPECT_TRUE(result == SUCCESSED);
                   if (result == Result::SUCCESSED) {
                     if (test_data.md5.length()) {
                       EXPECT_TRUE(test_data.md5 == base::GetFileMd5(test_data.target_file_path));
                     }
                   }
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
}

#if 0
TEST(BreakPointHttpTest, Http_ThreadNum_3_Breakpoint) {
  DoBreakpointTest(http_test_datas, 3);
}

TEST(BreakPointHttpTest, Http_ThreadNum_10_Breakpoint) {
  DoBreakpointTest(http_test_datas, 10);
}


// FTP

TEST(BreakPointFTPTest, FTP_ThreadNum_1_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 1);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_3_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 3);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_10_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 10);
}
#endif