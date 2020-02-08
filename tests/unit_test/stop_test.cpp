#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(StopTest, NoWait) {
  EXPECT_NO_THROW({
    Teemo t;
    t.Stop();
  });
}

TEST(StopTest, Wait) {
  EXPECT_NO_THROW({
    Teemo t;
    t.Stop(true);
  });
}