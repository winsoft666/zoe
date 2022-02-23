#include <future>
#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
using namespace teemo;

void DoTest(const std::vector<TestData>& test_datas, int thread_num, int32_t disk_cache) {
  Teemo::GlobalInit();

  for (const auto& test_data : test_datas) {
    Teemo efd;

    if (thread_num != -1)
      efd.setThreadNum(thread_num);

    efd.setDiskCacheSize(disk_cache);
    if (test_data.md5.length() > 0)
      efd.setHashVerifyPolicy(ALWAYS, MD5, test_data.md5);

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
  }

  Teemo::GlobalUnInit();
}

TEST(DiskCacheHttpTest, Http_ThreadNum_2) {
  DoTest(http_test_datas, 10, 10 * 1024 * 1024);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(DiskCacheHttpTest, Http_ThreadNum_3) {
  DoTest(http_test_datas, 3, 0);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST(DiskCacheHttpTest, Http_ThreadNum_20) {
  DoTest(http_test_datas, 20, 1);

  // set test case interval
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
