#pragma once
#include <string>
#include <vector>

typedef struct _TestData {
  std::string url;               // utf8
  std::string target_file_path;  // utf8
  std::string md5;               // utf8
} TestData;

static std::vector<TestData> http_test_datas = {
    {u8"https://d1.music.126.net/dmusic/NeteaseCloudMusic_Music_official_3.0.5.203184_64.exe", u8"./TeemoTest/NeteaseCloudMusic_Music_official_3.0.5.203184_64.exe", u8"2392d3bdc0726879da24b9b30d1b3faa"},
    {u8"https://dldir1.qq.com/qqfile/qq/QQNT/Mac/QQ_6.9.59_241104_01.dmg", u8"./TeemoTest/QQ_6.9.59_241104_01.dmg", u8"bc809f476bf822dac35ea5009475b9eb"},
};

static std::vector<TestData> ftp_test_datas = {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    {
        u8"ftp://speedtest.tele2.net/5MB.zip",
        u8"C:\\TeemoTest\\5MB.zip",
        u8"5f363e0e58a95f06cbe9bbc662c5dfb6",
    },
    {
        u8"ftp://speedtest.tele2.net/5MB.zip",
        u8"5MB.zip",
        u8"5f363e0e58a95f06cbe9bbc662c5dfb6",
    }
#else
    {
        u8"ftp://speedtest.tele2.net/5MB.zip",
        u8"~/TeemoTest/5MB.zip",
        u8"5f363e0e58a95f06cbe9bbc662c5dfb6",
    },
    {
        u8"ftp://speedtest.tele2.net/5MB.zip",
        u8"5MB.zip",
        u8"5f363e0e58a95f06cbe9bbc662c5dfb6",
    }
#endif
};