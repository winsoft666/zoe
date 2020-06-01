#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace teemo;

void DoBreakpointTest(std::vector<TestData> test_datas,
                      int thread_num,
                      bool enable_save_slice_to_tmp) {
  for (auto test_data : test_datas) {
    std::future<void> test_task = std::async(std::launch::async, [test_data, thread_num,
                                                                  enable_save_slice_to_tmp]() {
      Teemo efd;

      efd.SetThreadNum(thread_num);

      efd.Start(
          test_data.url, test_data.target_file_path,
          [test_data](Result result) {
            printf("\nResult: %s\n", GetResultString(result));
            EXPECT_TRUE(result == Successed || result == Canceled);
            if (result == Result::Successed) {
              if (test_data.md5.length()) {
                EXPECT_TRUE(test_data.md5 == base::GetFileMd5(test_data.target_file_path));
              }
            }
          },
          [](long total, long downloaded) {
            if (total > 0)
              printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
          },
          nullptr);

      std::this_thread::sleep_for(std::chrono::milliseconds(3000));

      efd.Stop(true);

      efd.SetThreadNum(thread_num);

      Result ret = efd.Start(
                   test_data.url, test_data.target_file_path,
                   [test_data](Result result) {
                     printf("\nResult: %s\n", GetResultString(result));
                     EXPECT_TRUE(result == Successed);
                     if (result == Result::Successed) {
                       if (test_data.md5.length()) {
                         EXPECT_TRUE(test_data.md5 == base::GetFileMd5(test_data.target_file_path));
                       }
                     }
                   },
                   [](long total, long downloaded) {
                     if (total > 0)
                       printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
                   },
                   nullptr)
                .get();
    });
    test_task.wait();
  }
}

TEST(BreakPointHttpTest, Http_ThreadNum_1_SliceToTmp_True_Breakpoint) {
  DoBreakpointTest(http_test_datas, 1, true);
}

TEST(BreakPointHttpTest, Http_ThreadNum_3_SliceToTmp_True_Breakpoint) {
  DoBreakpointTest(http_test_datas, 3, true);
}

TEST(BreakPointHttpTest, Http_ThreadNum_10_SliceToTmp_True_Breakpoint) {
  DoBreakpointTest(http_test_datas, 10, true);
}

TEST(BreakPointHttpTest, Http_ThreadNum_1_SliceToTmp_False_Breakpoint) {
  DoBreakpointTest(http_test_datas, 1, false);
}

TEST(BreakPointHttpTest, Http_ThreadNum_3_SliceToTmp_False_Breakpoint) {
  DoBreakpointTest(http_test_datas, 3, false);
}

TEST(BreakPointHttpTest, Http_ThreadNum_10_SliceToTmp_False_Breakpoint) {
  DoBreakpointTest(http_test_datas, 10, false);
}

// FTP

TEST(BreakPointFTPTest, FTP_ThreadNum_1_SliceToTmp_True_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 1, true);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_3_SliceToTmp_True_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 3, true);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_10_SliceToTmp_True_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 10, true);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_1_SliceToTmp_False_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 1, false);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_3_SliceToTmp_False_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 3, false);
}

TEST(BreakPointFTPTest, FTP_ThreadNum_10_SliceToTmp_False_Breakpoint) {
  DoBreakpointTest(ftp_test_datas, 10, false);
}
