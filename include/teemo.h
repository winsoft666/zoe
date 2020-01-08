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
#ifndef TEEMO_H_
#define TEEMO_H_
#pragma once
#include <string>
#include <memory>
#include "pplx/pplxtasks.h"

#ifdef TEEMO_STATIC
#define TEEMO_API
#else
#if defined(TEEMO_EXPORTS)
#if defined(_MSC_VER)
#define TEEMO_API __declspec(dllexport)
#else
#define TEEMO_API
#endif
#else
#if defined(_MSC_VER)
#define TEEMO_API __declspec(dllimport)
#else
#define TEEMO_API
#endif
#endif
#endif

namespace teemo {
enum Result {
  Successed = 0,
  UrlInvalid,
  TargetFilePathInvalid,
  ThreadNumInvalid,
  NetworkConnTimeoutInvalid,
  NetworkReadTimeoutInvalid,
  InternalNetworkError,
  GenerateTargetFileFailed,
  CleanupTmpFileFailed,
  AlreadyDownloading,
  Canceled,
  CanceledAndUpdateIndexFailed,
  Failed,
  FailedAndUpdateIndexFailed,
};

TEEMO_API const char *GetResultString(int enumVal);

typedef std::string utf8string;
typedef std::function<void(long total, long downloaded)> ProgressFunctor;
typedef std::function<void(long byte_per_sec)> RealtimeSpeedFunctor;
typedef std::function<void(const utf8string &verbose)> VerboseOuputFunctor;

class TEEMO_API Teemo {
public:
  Teemo();
  ~Teemo();

  static void GlobalInit();
  static void GlobalUnInit();

  void SetVerboseOutput(VerboseOuputFunctor verbose_functor);

  void SetSaveSliceFileToTempDir(bool enabled);
  bool IsSaveSliceFileToTempDir() const;

  Result SetThreadNum(size_t thread_num);
  size_t GetThreadNum() const;

  utf8string GetUrl() const;
  utf8string GetTargetFilePath() const;

  Result SetNetworkConnectionTimeout(size_t milliseconds); // default is 3000ms
  size_t GetNetworkConnectionTimeout() const;

  Result SetNetworkReadTimeout(size_t milliseconds); // default is 3000ms
  size_t GetNetworkReadTimeout() const;

  // default is -1 = forever, 0 = not use exist slice cache
  void SetSliceCacheExpiredTime(int seconds);

  int GetSliceCacheExpiredTime() const;

  void SetMaxDownloadSpeed(size_t byte_per_seconds); // default is 0 = not limit
  size_t GetMaxDownloadSpeed() const;

  pplx::task<Result> Start(const utf8string url, const utf8string &target_file_path,
                           ProgressFunctor progress_functor,
                           RealtimeSpeedFunctor realtime_speed_functor);

  void Stop(bool wait = false);

protected:
  class TeemoImpl;
  TeemoImpl* impl_;
};
} // namespace teemo

#endif