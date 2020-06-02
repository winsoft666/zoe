#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace teemo;

void DoTest(std::vector<TestData> test_datas, int thread_num) {
  Teemo::GlobalInit();

  for (auto test_data : test_datas) {
    Teemo efd;

    if (thread_num != -1)
      efd.setThreadNum(thread_num);

    efd.setDiskCacheSize(thread_num * 1 * 1024 * 1024);

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
               [](int64_t total, int64_t downloaded) {
                 if (total > 0)
                   printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
               },
               nullptr)
            .get();
  }

  Teemo::GlobalUnInit();
}

TEST(MultiThreadHttpTest, Http_DefaultThreadNum) {
  DoTest(http_test_datas, -1);
}

TEST(MultiThreadHttpTest, Http_ThreadNum_2) {
  DoTest(http_test_datas, 2);
}

TEST(MultiThreadHttpTest, Http_ThreadNum_3) {
  DoTest(http_test_datas, 3);
}

TEST(MultiThreadHttpTest, Http_ThreadNum_20) {
  DoTest(http_test_datas, 20);
}

// FTP

TEST(MultiThreadFTPTest, FTP_DefaultThreadNum) {
  DoTest(ftp_test_datas, -1);
}

TEST(MultiThreadFTPTest, FTP_ThreadNum_2) {
  DoTest(ftp_test_datas, 2);
}

TEST(MultiThreadFTPTest, FTP_ThreadNum_3) {
  DoTest(ftp_test_datas, 3);
}

TEST(MultiThreadFTPTest, FTP_ThreadNum_20) {
  DoTest(ftp_test_datas, 20);
}