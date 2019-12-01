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
    {
        "http://dl.softmgr.qq.com/original/Input/QQWubi_Setup_2.2.344.400.exe",
        "D:\\QQWubi_Setup_2.2.344.400.exe",
        "e635d2c510da6f17f3218683b4162b56",
    }
#else
    {
        "http://dl.softmgr.qq.com/original/Input/QQWubi_Setup_2.2.344.400.exe",
        "/root/QQWubi_Setup_2.2.344.400.exe",
        "e635d2c510da6f17f3218683b4162b56",
    }
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