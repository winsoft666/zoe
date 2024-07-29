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

#include "gtest/gtest.h"
#include "zoe/zoe.h"
#include "test_data.h"
#include <future>
using namespace zoe;

TEST(SingleTest, test1) {
  if (http_test_datas.size() == 0)
    return;
  TestData test_data = http_test_datas[0];

  Zoe::GlobalInit();
  {
    Zoe z;
    z.setVerifyCAEnabled(false, "");
    z.setThreadNum(6);
    z.setSlicePolicy(SlicePolicy::FixedNum, 10);
    if (test_data.md5.length() > 0)
      z.setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, test_data.md5);
    z.setFetchFileInfoHeadMethodEnabled(true);
    z.setHttpHeaders({ {"User-Agent", "Zoe"}});

    std::shared_future<ZoeResult> future_result = z.start(
        test_data.url, test_data.target_file_path,
        [](ZoeResult result) {
          printf("\nResult: %s\n", Zoe::GetResultString(result));
          EXPECT_TRUE(result == ZoeResult::SUCCESSED);
        },
        [](int64_t total, int64_t downloaded) {
          if (total > 0)
            printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
        },
        nullptr);

    EXPECT_TRUE(future_result.get() == ZoeResult::SUCCESSED);
    DownloadState s = z.state();
  }
  Zoe::GlobalUnInit();
}
