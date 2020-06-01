#include <future>

#include "../md5.h"
#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
using namespace teemo;

void DoTest(std::vector<TestData> test_datas,
            int thread_num,
            bool enable_save_slice_to_tmp,
            size_t disk_cache) {
  Teemo::GlobalInit();

  for (auto test_data : test_datas) {
    Teemo efd;

    if (thread_num != -1)
      efd.SetThreadNum(thread_num);

    efd.SetDiskCacheSize(disk_cache);

    Result ret =
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
               nullptr)
            .get();
  }

  Teemo::GlobalUnInit();
}

TEST(DiskCacheHttpTest, Http_ThreadNum_2_SliceToTmp_Flase) {
  DoTest(http_test_datas, 10, false, 10 * 1024 * 1024);
}

TEST(DiskCacheHttpTest, Http_ThreadNum_3_SliceToTmp_Flase) {
  DoTest(http_test_datas, 3, false, 0);
}

TEST(DiskCacheHttpTest, Http_ThreadNum_20_SliceToTmp_Flase) {
  DoTest(http_test_datas, 20, false, 1);
}
