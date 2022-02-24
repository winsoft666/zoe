#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
#include <vector>
using namespace teemo;

TEST(ConcurrentTest, test1) {
  if (http_test_datas.size() < 2)
    return;

  Teemo::GlobalInit();
  {
    std::vector<std::shared_ptr<Teemo>> efds;
    for (size_t i = 0; i < http_test_datas.size(); i++) {
      std::shared_ptr<Teemo> t = std::make_shared<Teemo>();
      efds.push_back(t);

      t->setThreadNum(6);
      t->setSlicePolicy(SlicePolicy::FixedNum, 10);
      if (http_test_datas[i].md5.length() > 0)
        t->setHashVerifyPolicy(ALWAYS, MD5, http_test_datas[i].md5);

      t->start(
          http_test_datas[i].url, http_test_datas[i].target_file_path,
          [i](Result result) {
            printf("\n[%d] Result: %s\n", i, GetResultString(result));
            EXPECT_TRUE(result == SUCCESSED);
          },
          [i](int64_t total, int64_t downloaded) {
            if (total > 0)
              printf("[%d] %3d%%\b\b\b\b\b\b\b\b", i,
                     (int)((double)downloaded * 100.f / (double)total));
          },
          nullptr);
    }

    for (size_t i = 0; i < efds.size(); i++) {
      efds[i]->futureResult().wait();
    }
  }
  Teemo::GlobalUnInit();
}
