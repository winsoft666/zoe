#include "gtest/gtest.h"
#include "teemo.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace teemo;

void DoTest(std::vector<TestData> test_datas, int thread_num, bool enable_save_slice_to_tmp) {
  Teemo::GlobalInit();

  for (auto test_data : test_datas) {

    Teemo efd;

    if (thread_num != -1)
      efd.SetThreadNum(thread_num);

    efd.SetSaveSliceFileToTempDir(enable_save_slice_to_tmp);

    efd.Start(
           test_data.url, test_data.target_file_path,
           [](long total, long downloaded) {
             if (total > 0)
               printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
           },
           nullptr)
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

  Teemo::GlobalUnInit();
}

TEST(MultiThreadHttpTest, Http_DefaultThreadNum_SliceToTmp_Flase) {
  DoTest(http_test_datas, -1, false);
}

TEST(MultiThreadHttpTest, Http_ThreadNum_2_SliceToTmp_Flase) { DoTest(http_test_datas, 2, false); }

TEST(MultiThreadHttpTest, Http_ThreadNum_3_SliceToTmp_Flase) { DoTest(http_test_datas, 3, false); }

TEST(MultiThreadHttpTest, Http_ThreadNum_20_SliceToTmp_Flase) {
  DoTest(http_test_datas, 20, false);
}

TEST(MultiThreadHttpTest, Http_DefaultThreadNum_SliceToTmp_True) {
  DoTest(http_test_datas, -1, true);
}

TEST(MultiThreadHttpTest, Http_ThreadNum_2_SliceToTmp_True) { DoTest(http_test_datas, 2, true); }

TEST(MultiThreadHttpTest, Http_ThreadNum_3_SliceToTmp_True) { DoTest(http_test_datas, 3, true); }

// FTP

TEST(MultiThreadFTPTest, FTP_DefaultThreadNum_SliceToTmp_Flase) {
  DoTest(ftp_test_datas, -1, false);
}

TEST(MultiThreadFTPTest, FTP_ThreadNum_2_SliceToTmp_Flase) { DoTest(ftp_test_datas, 2, false); }

TEST(MultiThreadFTPTest, FTP_ThreadNum_3_SliceToTmp_Flase) { DoTest(ftp_test_datas, 3, false); }

TEST(MultiThreadFTPTest, FTP_ThreadNum_20_SliceToTmp_Flase) { DoTest(ftp_test_datas, 20, false); }

TEST(MultiThreadFTPTest, FTP_DefaultThreadNum_SliceToTmp_True) { DoTest(ftp_test_datas, -1, true); }

TEST(MultiThreadFTPTest, FTP_ThreadNum_2_SliceToTmp_True) { DoTest(ftp_test_datas, 2, true); }

TEST(MultiThreadFTPTest, FTP_ThreadNum_3_SliceToTmp_True) { DoTest(ftp_test_datas, 3, true); }
