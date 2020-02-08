#pragma once
#include <string>
#include <vector>

typedef struct _TestData {
  std::string url;  // utf8
  std::string target_file_path; // utf8
  std::string md5; // utf8
} TestData;

static std::vector<TestData> http_test_datas = {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    {u8"https://dlie.sogoucdn.com/se/sogou_explorer_8.6_1120.exe",
     u8"D:\\TeemoTest\\sogou_explorer_8.6_1120.exe", u8"8e78e77400cdc268032a5491d2fe18d8"},
    {u8"https://dlie.sogoucdn.com/se/sogou_explorer_8.6_1120.exe",
     u8"D:\\TeemoTest\\ËÑ¹·ä¯ÀÀÆ÷_8.6_1120.exe", u8"8e78e77400cdc268032a5491d2fe18d8"}
#else
    {u8"https://dlie.sogoucdn.com/se/sogou_explorer_8.6_1120.exe",
     u8"~/TeemoTest/sogou_explorer_8.6_1120.exe", u8"8e78e77400cdc268032a5491d2fe18d8"},
    {u8"https://dlie.sogoucdn.com/se/sogou_explorer_8.6_1120.exe",
     u8"~/TeemoTest/ËÑ¹·ä¯ÀÀÆ÷_8.6_1120.exe", u8"8e78e77400cdc268032a5491d2fe18d8"}
#endif
};

static std::vector<TestData> ftp_test_datas = {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    {
        u8"ftp://speedtest.tele2.net/5MB.zip",
        u8"D:\\TeemoTest\\5MB.zip",
        u8"5f363e0e58a95f06cbe9bbc662c5dfb6",
    }
#else
    {
        u8"ftp://speedtest.tele2.net/5MB.zip",
        u8"~/TeemoTest/5MB.zip",
        u8"5f363e0e58a95f06cbe9bbc662c5dfb6",
    }
#endif
};