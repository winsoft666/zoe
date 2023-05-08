/*******************************************************************************
*    Copyright (C) <2019-2022>, winsoft666, <winsoft666@outlook.com>.
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

#include <iostream>
#include <string.h>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include "libGet/libGet.h"
#include <mutex>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#else
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#endif

using namespace LIBGET_NAMESPACE;
LIBGET efd;
std::mutex console_mutex;

void PrintConsole(int64_t total, int64_t downloaded, int32_t speed);

inline char EasyCharToLowerA(char in) {
  if (in <= 'Z' && in >= 'A')
    return in - ('Z' - 'z');
  return in;
}

template <typename T, typename Func>
typename std::enable_if<std::is_same<char, T>::value || std::is_same<wchar_t, T>::value,
                        std::basic_string<T, std::char_traits<T>, std::allocator<T>>>::type
StringCaseConvert(const std::basic_string<T, std::char_traits<T>, std::allocator<T>>& str,
                  Func func) {
  std::basic_string<T, std::char_traits<T>, std::allocator<T>> ret = str;
  std::transform(ret.begin(), ret.end(), ret.begin(), func);
  return ret;
}

#if (defined WIN32 || defined _WIN32)
BOOL WINAPI ControlSignalHandler(DWORD fdwCtrlType) {
  switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      efd.stop();
      return TRUE;

    default:
      return FALSE;
  }
}
#else

void ControlSignalHandler(int s) {
  efd.stop();
}
#endif

//
// Usage:
// libGet_tool URL TargetFilePath [ThreadNum] [DiskCacheMb] [MD5] [TmpExpiredSeconds] [MaxSpeed]
//
int main(int argc, char** argv) {
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

  char* url = argv[1];
  char* target_file_path = argv[2];

  if (argc >= 4)
    efd.setThreadNum(atoi(argv[3]));
  if (argc >= 5)
    efd.setDiskCacheSize(atoi(argv[4]) * 1024 * 1024);
  if (argc >= 6) {
    if (strlen(argv[5]) > 0) {
      efd.setHashVerifyPolicy(ALWAYS, MD5, argv[5]);
    }
  }
  if (argc >= 7)
    efd.setTmpFileExpiredTime(atoi(argv[6]));
  if (argc >= 8)
    efd.setMaxDownloadSpeed(atoi(argv[7]));

  int exit_code = 0;
  LIBGET::GlobalInit();
  FILE* f_verbose = fopen("libGet_tool_verbose.log", "wb");
  efd.setVerboseOutput([f_verbose](const utf8string& verbose) {
    fwrite(verbose.c_str(), 1, verbose.size(), f_verbose);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    OutputDebugStringA(verbose.c_str());
#endif
  });

  auto start_time = std::chrono::high_resolution_clock::now();
  std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

  std::shared_future<Result> aysnc_task = efd.start(
      url, target_file_path,
      [=, &exit_code, &end_time](Result result) {
        std::cout << std::endl << GetResultString(result) << std::endl;
        exit_code = result;

        end_time = std::chrono::high_resolution_clock::now();
        if (result == Result::SUCCESSED) {
        }
      },
      [](int64_t total, int64_t downloaded) { PrintConsole(total, downloaded, -1); },
      [](int64_t byte_per_secs) { PrintConsole(-1, -1, byte_per_secs / 1000.f); });

  aysnc_task.wait();

  std::chrono::milliseconds mill =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  std::cout << "Total: " << mill.count() << "ms" << std::endl;

  fclose(f_verbose);
  LIBGET::GlobalUnInit();
  return exit_code;
}

void PrintConsole(int64_t total, int64_t downloaded, int32_t speed) {
  const char* PBSTR = "============================================================";
  const int PBWIDTH = 60;

  std::lock_guard<std::mutex> lg(console_mutex);

  static int64_t avaliable_total = -1;
  static int64_t avaliable_downloaded = -1;
  static int32_t avaliable_speed = 0;

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