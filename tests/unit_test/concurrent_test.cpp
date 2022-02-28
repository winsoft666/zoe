/*******************************************************************************
*    Copyright (C) <2019-2022>, winsoft666, <winsoft666@outlook.com>.
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

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
