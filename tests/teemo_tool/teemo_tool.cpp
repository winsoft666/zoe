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
#include <string.h>
#include <sstream>
#include <iomanip>
#include "teemo.h"
#include "../md5.h"
#include <mutex>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#else
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#endif

using namespace teemo;
Teemo efd;
std::mutex console_mutex;

void PrintConsole(long total, long downloaded, long speed);

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

void ControlSignalHandler(int s) { efd.Stop(true); }
#endif

//
// Usage:
// easy_download_tool URL TargetFilePath [ThreadNum] [MD5] [EnableSaveSliceToTmp] [SliceCacheExpiredSeconds] [MaxSpeed]
//
int main(int argc, char **argv) {
  if (argc < 3) {
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
  sigaction(SIGQUIT, &sigIntHandler, NULL);
#endif

  char *url = argv[1];
  char *target_file_path = argv[2];
  char *md5 = nullptr;

  if (argc >= 4)
    efd.SetThreadNum(atoi(argv[3]));
  if (argc >= 5)
    md5 = argv[4];
  if (argc >= 6)
    efd.SetSaveSliceFileToTempDir((atoi(argv[5]) == 1));
  if (argc >= 7)
    efd.SetSliceCacheExpiredTime(atoi(argv[6]));
  if (argc >= 8)
    efd.SetMaxDownloadSpeed(atoi(argv[7]));

  int exit_code = 0;
  Teemo::GlobalInit();
  FILE *f_verbose = fopen("teemo_tool_verbose.log", "wb");
  efd.SetVerboseOutput([f_verbose](const utf8string &verbose) { 
    fwrite(verbose.c_str(), 1, verbose.size(), f_verbose);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    OutputDebugStringA(verbose.c_str());
#endif
  });

  efd.Start(
         url, target_file_path,
         [](long total, long downloaded) { PrintConsole(total, downloaded, -1); },
         [](long byte_per_secs) { PrintConsole(-1, -1, byte_per_secs / 1000); })
      .then([=, &exit_code](pplx::task<Result> result) {
        std::cout << std::endl << GetResultString(result.get()) << std::endl;
        exit_code = result.get();

        if (result.get() == Result::Successed) {
          if (md5) {
            if (strcmp(md5, ppx::base::GetFileMd5(target_file_path).c_str()) == 0) {
              std::cout << "MD5 checksum successful." << std::endl;
            }
            else {
              exit_code = -1;
              std::cout << "MD5 checksum failed." << std::endl;
            }
          }
        }
      })
      .wait();

  fclose(f_verbose);
  Teemo::GlobalUnInit();
  std::cout << "Global UnInit." << std::endl;
  return exit_code;
}

void PrintConsole(long total, long downloaded, long speed) {
  const char *PBSTR = "============================================================";
  const int PBWIDTH = 60;

  std::lock_guard<std::mutex> lg(console_mutex);

  static long avaliable_total = -1;
  static long avaliable_downloaded = -1;
  static long avaliable_speed = 0;

  if (total > 0)
    avaliable_total = total;

  if (downloaded > 0)
    avaliable_downloaded = downloaded;

  double percentage = 0;
  if (avaliable_total > 0 && avaliable_downloaded >= 0)
    percentage = (double)avaliable_downloaded / (double)avaliable_total;

  if (speed >= 0)
    avaliable_speed = speed;

  int val = (int)(percentage * 100);
  int lpad = (int)(percentage * PBWIDTH);
  int rpad = PBWIDTH - lpad;

  if (val < 0 || val > 100)
    return;

  std::stringstream ss_buf;
  if (avaliable_downloaded >= 1073741824)
    ss_buf << std::fixed << std::setprecision(1) << (double)avaliable_downloaded / 1073741824.f
           << "G";
  else if (avaliable_downloaded >= 1048576)
    ss_buf << std::fixed << std::setprecision(1) << (double)avaliable_downloaded / 1048576 << "M";
  else if (avaliable_downloaded >= 1024)
    ss_buf << std::fixed << std::setprecision(1) << (double)avaliable_downloaded / 1024 << "K";
  else if (avaliable_downloaded == -1)
    ss_buf << "-";
  else
    ss_buf << avaliable_downloaded << "b";

  ss_buf << " / ";

  if (avaliable_total >= 1073741824)
    ss_buf << std::fixed << std::setprecision(1) << (double)avaliable_total / 1073741824.f << "G";
  else if (avaliable_total >= 1048576)
    ss_buf << std::fixed << std::setprecision(1) << (double)avaliable_total / 1048576.f << "M";
  else if (avaliable_total >= 1024)
    ss_buf << std::fixed << std::setprecision(1) << (double)avaliable_total / 1024.f << "K";
  else if (avaliable_total == -1)
    ss_buf << "-";
  else
    ss_buf << avaliable_total << "b";

  if (val == 100 || val == 0) {
    printf("\r[%.*s%*s] %-14s %3d%% %8d kb/s", lpad, PBSTR, rpad, "", ss_buf.str().c_str(), val,
           avaliable_speed);
  }
  else {
    printf("\r[%.*s>%*s] %-14s %3d%% %8d kb/s", lpad, PBSTR, rpad - 1, "", ss_buf.str().c_str(),
           val, avaliable_speed);
  }
  fflush(stdout);
}