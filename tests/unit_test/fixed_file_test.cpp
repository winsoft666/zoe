#include "gtest/gtest.h"
#include "file_util.h"
using namespace teemo;

TEST(FixedFileTest, 100mb) {
  const int file_size = 100 * 1024 * 1024;
  EXPECT_TRUE(FileUtil::CreateFixedSizeFile("fixed_test.dat", file_size));
  EXPECT_EQ(FileUtil::GetFileSize("fixed_test.dat"), file_size);
}

TEST(FixedFileTest, 2000mb) {
  const int file_size = 2000 * 1024 * 1024;
  EXPECT_TRUE(FileUtil::CreateFixedSizeFile("fixed_test.dat", file_size));
  EXPECT_EQ(FileUtil::GetFileSize("fixed_test.dat"), file_size);
}

TEST(FixedFileTest, 1mb) {
  const int file_size = 1 * 1024 * 1024;
  EXPECT_TRUE(FileUtil::CreateFixedSizeFile("fixed_test.dat", file_size));
  EXPECT_EQ(FileUtil::GetFileSize("fixed_test.dat"), file_size);
}

TEST(FixedFileTest, 10byte) {
  const int file_size = 10;
  EXPECT_TRUE(FileUtil::CreateFixedSizeFile("fixed_test.dat", file_size));
  EXPECT_EQ(FileUtil::GetFileSize("fixed_test.dat"), file_size);
}

TEST(FixedFileTest, 0byte) {
  const int file_size = 0;
  EXPECT_TRUE(FileUtil::CreateFixedSizeFile("fixed_test.dat", file_size));
  EXPECT_EQ(FileUtil::GetFileSize("fixed_test.dat"), file_size);
}