/*******************************************************************************
* Copyright (C) 2019 - 2023, winsoft666, <winsoft666@outlook.com>.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
*
* Expect bugs
*
* Please use and enjoy. Please let me know of any bugs/improvements
* that you have found/implemented and I will fix/incorporate them into this
* file.
*******************************************************************************/

#include <iostream>
#include "easy_file_download.h"
#include "../md5.h"
#include <mutex>
#if (defined WIN32 || defined _WIN32)
    #include <windows.h>
#else
    #include <signal.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <unistd.h>
#endif

using namespace easy_file_download;
EasyFileDownload efd;
std::mutex console_mutex;

void PrintConsole(double percentage, long speed);

#if (defined WIN32 || defined _WIN32)
BOOL WINAPI ControlSignalHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            efd.Stop(true);
            return TRUE;

        default:
            return FALSE;
    }
}
#else

void ControlSignalHandler(int s) {
    printf("Caught signal %d\n", s);
    exit(1);
}
#endif



//
// Usage:
// easy_download_tool thread_num url target_file_path [md5]
//
int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "Argument Number Error\n";
        return 1;
    }

#if (defined WIN32 || defined _WIN32)
    SetConsoleCtrlHandler(ControlSignalHandler, TRUE);
#else
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ControlSignalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
#endif

    int thread_num = atoi(argv[1]);
    char *url = argv[2];
    char *target_file_path = argv[3];
    char *md5 = nullptr;
    if(argc >= 5)
        md5 = argv[4];

    EasyFileDownload::GlobalInit();

    efd.Start(true,
              thread_num,
              url,
              target_file_path,
    [](long total, long downloaded) {
        PrintConsole((double)downloaded / (double)total, -1);
    }, [](long byte_per_secs) {
        PrintConsole(-1, byte_per_secs / 1000);
    }).then([ = ](pplx::task<Result> result) {
        std::cout << std::endl << GetResultString(result.get()) << std::endl;
        if (result.get() == Result::Success) {
            if (md5) {
                if (strcmp(md5, ppx::base::GetFileMd5(target_file_path).c_str()) == 0)
                    std::cout << "MD5 checksum successful" << std::endl;
                else
                    std::cout << "MD5 checksum failed" << std::endl;
            }
        }
    }).wait();

    EasyFileDownload::GlobalUnInit();
    std::cout << "Global UnInit" << std::endl;
    return 0;
}


void PrintConsole(double percentage, long speed) {
    std::lock_guard<std::mutex> lg(console_mutex);

    static double last_percentage = 0;
    static long last_speed = 0;

    if(percentage != -1)
        last_percentage = percentage;

    if(speed != -1)
        last_speed = speed;

    const char *PBSTR =
        "============================================================";
    const int PBWIDTH = 60;
    int val = (int)(last_percentage * 100);
    int lpad = (int)(last_percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;

    if (val < 0 || val > 100)
        return;

    if (val == 100 || val == 0) {
        printf("\r[%.*s%*s] %3d%% %8d kb/s", lpad, PBSTR, rpad, "", val, last_speed);
    } else {
        printf("\r[%.*s>%*s] %3d%% %8d kb/s", lpad, PBSTR, rpad - 1, "", val, last_speed);
    }
    fflush(stdout);
}