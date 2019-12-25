#include "gtest/gtest.h"
#include "easy_file_download.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace easy_file_download;

void DoBreakpointTest(std::vector<TestData> test_datas, int thread_num,
                      bool enable_save_slice_to_tmp) {
  for (auto test_data : test_datas) {
    std::future<void> test_task =
        std::async(std::launch::async, [test_data, thread_num, enable_save_slice_to_tmp]() {
          EasyFileDownload efd;

          efd.SetThreadNum(thread_num);
          efd.SetEnableSaveSliceFileToTempDir(enable_save_slice_to_tmp);

          efd.Start(test_data.url, test_data.target_file_path, nullptr, nullptr)
              .then([=](pplx::task<Result> result) {
                EXPECT_TRUE(result.get() == Successed || result.get() == Canceled);
                if (result.get() == Result::Successed) {
                  if (test_data.md5.length()) {
                    EXPECT_TRUE(test_data.md5 == ppx::base::GetFileMd5(test_data.target_file_path));
                  }
                }
              });

          std::this_thread::sleep_for(std::chrono::milliseconds(3000));

          efd.Stop(true);

          efd.SetThreadNum(thread_num);
          efd.SetEnableSaveSliceFileToTempDir(enable_save_slice_to_tmp);

          efd.Start(test_data.url, test_data.target_file_path, nullptr, nullptr)
              .then([=](pplx::task<Result> result) {
                EXPECT_TRUE(result.get() == Successed);
                if (result.get() == Result::Successed) {
                  if (test_data.md5.length()) {
                    EXPECT_TRUE(test_data.md5 == ppx::base::GetFileMd5(test_data.target_file_path));
                  }
                }
              })
              .wait();
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
