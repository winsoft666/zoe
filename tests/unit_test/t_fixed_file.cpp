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
#include "file_util.h"
using namespace zoe;

TEST_CASE("FixedFileTest-100MB") {
  const int file_size = 100 * 1024 * 1024;
  REQUIRE(FileUtil::CreateFixedSizeFile("fixed_test0.dat", file_size));
  REQUIRE(FileUtil::GetFileSize("fixed_test0.dat") == file_size);
}

TEST_CASE("FixedFileTest-2000MB") {
  const int file_size = 2000 * 1024 * 1024;
  REQUIRE(FileUtil::CreateFixedSizeFile("fixed_test1.dat", file_size));
  REQUIRE(FileUtil::GetFileSize("fixed_test1.dat") == file_size);
}

TEST_CASE("FixedFileTest-1MB") {
  const int file_size = 1 * 1024 * 1024;
  REQUIRE(FileUtil::CreateFixedSizeFile("fixed_test2.dat", file_size));
  REQUIRE(FileUtil::GetFileSize("fixed_test2.dat") == file_size);
}

TEST_CASE("FixedFileTest-10byte") {
  const int file_size = 10;
  REQUIRE(FileUtil::CreateFixedSizeFile("fixed_test3.dat", file_size));
  REQUIRE(FileUtil::GetFileSize("fixed_test3.dat") == file_size);
}

TEST_CASE("FixedFileTest-0byte") {
  const int file_size = 0;
  REQUIRE(FileUtil::CreateFixedSizeFile("fixed_test4.dat", file_size));
  REQUIRE(FileUtil::GetFileSize("fixed_test4.dat") == file_size);
}