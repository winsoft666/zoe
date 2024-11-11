/*******************************************************************************
*    Copyright (C) <2019-2024>, winsoft666, <winsoft666@outlook.com>.
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

#include "catch.hpp"
#include "zoe/zoe.h"
#include "test_data.h"
#include <future>
#include <vector>
using namespace zoe;

TEST_CASE("ConcurrentTest") {
  if (http_test_datas.size() < 2)
    return;

  Zoe::GlobalInit();
  {
    std::vector<std::shared_ptr<Zoe>> efds;
    for (size_t i = 0; i < http_test_datas.size(); i++) {
      std::shared_ptr<Zoe> t = std::make_shared<Zoe>();
      efds.push_back(t);

      t->setThreadNum(6);
      t->setSlicePolicy(SlicePolicy::FixedNum, 10);
      if (http_test_datas[i].md5.length() > 0)
        t->setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, http_test_datas[i].md5);

      t->start(
          http_test_datas[i].url, http_test_datas[i].target_file_path,
          [i](ZoeResult result) {
            printf("\n[%d] ZoeResult: %s\n", i, Zoe::GetResultString(result));
            REQUIRE(result == ZoeResult::SUCCESSED);
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
  Zoe::GlobalUnInit();
}
