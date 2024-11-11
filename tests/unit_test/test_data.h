#pragma once
#include <string>
#include <vector>

typedef struct _TestData {
  std::string url;               // utf8
  std::string target_file_path;  // utf8
  std::string md5;               // utf8
} TestData;

static std::vector<TestData> http_test_datas = {
    {u8"https://mksoftcdnhp.mydown.com/6731d49b/f44008af4a4ef1a5f387811565b337e8/uploadsoft/douyin-downloader-v4.9.0-win32-ia32-tjjh.exe", u8"./TeemoTest/douyin-downloader-v4.9.0-win32-ia32-tjjh.exe", u8"4deb7ed6ba1f3cb82000779db85d016f"},
    {u8"https://mksoftcdnhp.mydown.com/6731d4fb/d6639ac1aad76ea3fbc208c1331699c3/uploadsoft/newsoft/QQWubi_Setup_2.4.629.400.exe", u8"./TeemoTest/QQWubi_Setup_2.4.629.400.exe", u8"7cf1310e14a479c7c184fb3dc28d1585"},
    {u8"https://mksoftcdnhp.mydown.com/6731d535/d1492daadc289e1fec084b97fbf3ac24/uploadsoft/XunLeiWebSetup12.0.14.2602xl11.exe", u8"./TeemoTest/XunLeiWebSetup12.0.14.2602xl11.exe", u8"2d6a21ca175afa7f356440efeedc97e8"},
    {u8"https://mksoftcdnhp.mydown.com/6731d55d/5303f8aa64f33952a0c2040e4244285f/uploadsoft/bmsetup-30.0.31.9.exe", u8"./TeemoTest/bmsetup-30.0.31.9.exe", u8"2d0e4afe072b86ca0ac4528127b05f15"},
    {u8"http://cosbrowser-1253960454.cos.ap-shanghai.myqcloud.com/releases/cosbrowser-setup-2.11.23.exe", u8"./TeemoTest/cosbrowser-setup-2.11.23.exe", u8"c176bd79f4b464cf5562ae0b34fe9b10"},
    {u8"http://dl.todesk.com/xp/ToDesk_4.7.5.3_xp.exe", u8"./TeemoTest/ToDesk_4.7.5.3_xp.exe", u8"cdb4222cea387e249ba189b9a7b25b50"},
    {u8"http://download.2345.com/union7139_2345/2345explorer_50139539532.exe", u8"./TeemoTest/2345explorer_50139539532.exe", u8"79dd5d277259975e0211c52a554f4f19"},
    {u8"http://mksoftcdnhp.mydown.com/67319728/a0d9a058627d7e1667c686120ad5b371/uploadsoft/QQGameMini_1080000167_cid0.exe", u8"./TeemoTest/QQGameMini_1080000167_cid0.exe", u8"175778ccbdeb35539472f7955632f9d5"},
    {u8"http://mksoftcdnhp.mydown.com/67319772/5088465f7efba6688d62b1fc6833799e/uploadsoft/ChromeSetup130.0.6723.117.exe", u8"./TeemoTest/ChromeSetup130.0.6723.117.exe", u8"47399831a513265b24c04b44a5ab6661"},
    {u8"http://mksoftcdnhp.mydown.com/673197a8/5825b63eb334069bee97e7a7cfd7a47d/uploadsoft/newsoft/QQPCDownload1122072708.exe", u8"./TeemoTest/QQPCDownload1122072708.exe", u8"0042b8d48604bb80dca2b30c363275a5"},
    {u8"http://mksoftcdnhp.mydown.com/673197d5/819998b51eb0e3195ae1fe8ebf1a6c30/uploadsoft/JZRecordCovIns_014_410.exe", u8"./TeemoTest/JZRecordCovIns_014_410.exe", u8"b70b8baad30ebd32e97572c7e421ba6f"},

};

inline TestData GetHttpTestData() {
    static size_t idx = 0;
    assert(http_test_datas.size() > 0);
    if (http_test_datas.size() == 0)
        return TestData();

    if (idx >= http_test_datas.size())
        idx = 0;

    return http_test_datas[idx++];
}

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