#include "gtest/gtest.h"
#include "teemo/teemo.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(StopTest, NoWait) {
  EXPECT_NO_THROW({
    Teemo t;
    t.stop();
  });
}