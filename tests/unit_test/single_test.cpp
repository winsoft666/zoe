#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(SingleTest, test1) {
  Teemo::GlobalInit();

  Teemo efd;
  efd.setThreadNum(3);


  efd.start(
      "http://n.sinaimg.cn/sports/2_img/dfic/cf0d0fdd/83/w1024h659/20200728/a854-iwxpesx4871320.jpg", "D:\\a854-iwxpesx4871320.jpg",
      [](Result result) {
        printf("\nResult: %s\n", GetResultString(result));
        EXPECT_TRUE(result == SUCCESSED || result == CANCELED);
      },
      [](int64_t total, int64_t downloaded) {
        if (total > 0)
          printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
      },
      nullptr);

  system("pause");

  Teemo::GlobalUnInit();
}
