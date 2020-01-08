#include "gtest/gtest.h"
#include "teemo.h"
#include "../md5.h"
#include "test_data.h"
#include <future>
using namespace teemo;

TEST(StopTest, NoWait) {
  Teemo t;
  bool has_exception = false;
  try {
    t.Stop();
  } catch (...) {
    has_exception = true;
  }
  EXPECT_FALSE(has_exception);
}

TEST(StopTest, Wait) {
  Teemo t;
  bool has_exception = false;
  try {
    t.Stop(true);
  } catch (...) {
    has_exception = true;
  }
  EXPECT_FALSE(has_exception);
}