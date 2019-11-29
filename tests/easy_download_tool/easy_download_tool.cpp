#include <iostream>
#include "easy_file_download.h"
#include "../md5.h"
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

void PrintConsoleProcess(double percentage);

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

    efd.Start(thread_num,
              url,
              target_file_path,
    [](long total, long downloaded) {
        PrintConsoleProcess((double)downloaded / (double)total);
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


void PrintConsoleProcess(double percentage) {
    const char *PBSTR =
        "============================================================";
    const int PBWIDTH = 60;
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;

    if (val < 0 || val > 100)
        return;

    if (val == 100 || val == 0) {
        printf("\r[%.*s%*s] %3d%%", lpad, PBSTR, rpad, "", val);
    } else {
        printf("\r[%.*s>%*s] %3d%%", lpad, PBSTR, rpad - 1, "", val);
    }
    fflush(stdout);
}
