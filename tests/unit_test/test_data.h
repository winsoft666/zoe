#pragma once
#include <string>
#include <vector>

typedef struct _TestData {
  std::string url;
  std::string target_file_path;
  std::string md5;
} TestData;

static std::vector<TestData> http_test_datas = {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    {"https://dlie.sogoucdn.com/se/sogou_explorer_8.6_1120.exe", "D:\\sogou_explorer_8.6_1120.exe",
     "8e78e77400cdc268032a5491d2fe18d8"}
#else
    {"https://dlie.sogoucdn.com/se/sogou_explorer_8.6_1120.exe", "D:\\sogou_explorer_8.6_1120.exe",
     "8e78e77400cdc268032a5491d2fe18d8"}
#endif
};

static std::vector<TestData> ftp_test_datas = {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    {
        "ftp://speedtest.tele2.net/5MB.zip",
        "D:\\5MB.zip",
        "5f363e0e58a95f06cbe9bbc662c5dfb6",
    }
#else
    {
        "ftp://speedtest.tele2.net/5MB.zip",
        "/root/5MB.zip",
        "5f363e0e58a95f06cbe9bbc662c5dfb6",
    }
#endif
};