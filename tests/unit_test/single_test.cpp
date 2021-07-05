#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(SingleTest, test1) {
  Teemo::GlobalInit();

  Teemo efd;
  efd.setThreadNum(6);
  efd.setSlicePolicy(SlicePolicy::FixedSize, 1024000 * 5);
  efd.setHashVerifyPolicy(ALWAYS, MD5, "cfc86ceb95503c7251941a4da0ce13a6");
  efd.setHttpHeaders({{u8"Origin", u8"https://mysql.com"},
                      {u8"User-Agent", u8"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)"}});

  efd.start(
      "https://cdn.mysql.com//Downloads/MySQL-8.0/mysql-8.0.23-winx64-debug-test.zip",
      "mysql.zip",
      [](Result result) {
        printf("\nResult: %s\n", GetResultString(result));
        EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
      },
      [](int64_t total, int64_t downloaded) {
        if (total > 0)
          printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
      },
      nullptr);

  //std::this_thread::sleep_for(std::chrono::milliseconds(500));

  getchar();

  bool stop_ret = efd.stop(-1);
  printf("Stop result: %s\n", stop_ret ? "true" : "false");

  getchar();

  Teemo::GlobalUnInit();
}
