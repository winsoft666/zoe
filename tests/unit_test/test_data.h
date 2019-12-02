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
        "https://download.skype.com/s4l/download/win/Skype-8.54.0.91.exe",
        "D:\\Skype-8.54.0.91.exe",
        "bf593256fd9faafe68a507a8151b1f29"
    }
#else
    {
        "https://download.skype.com/s4l/download/win/Skype-8.54.0.91.exe",
        "D:\\Skype-8.54.0.91.exe",
        "bf593256fd9faafe68a507a8151b1f29"
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