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
#include <atomic>
#include <future>

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
  QueryFileSizeRetryTimesInvalid,
  InternalNetworkError,
  GenerateTargetFileFailed,
  CleanupTmpFileFailed,
  AlreadyDownloading,
  Canceled,
  CanceledAndUpdateIndexFailed,
  Failed,
  FailedAndUpdateIndexFailed,
  GetSliceDirectoryFailed,
  CreateSliceDirectoryFailed,
  OpenSliceFileFailed,
  CreateSliceIndexDirectoryFailed
};

TEEMO_API const char* GetResultString(int enumVal);

class TEEMO_API CancelEvent {
 public:
  CancelEvent();
  ~CancelEvent();

  void Cancel() noexcept;
  void UnCancel() noexcept;
  bool IsCanceled() noexcept;

 protected:
  CancelEvent(const CancelEvent&) = delete;
  CancelEvent& operator=(const CancelEvent&) = delete;
  std::atomic<bool> canceled_;
};

typedef std::string utf8string;
typedef std::function<void(Result ret)> ResultFunctor;
typedef std::function<void(long total, long downloaded)> ProgressFunctor;
typedef std::function<void(long byte_per_sec)> RealtimeSpeedFunctor;
typedef std::function<void(const utf8string& verbose)> VerboseOuputFunctor;

class TEEMO_API Teemo {
 public:
  Teemo();
  ~Teemo();

  static void GlobalInit();
  static void GlobalUnInit();

  void SetVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept;

  void SetSaveSliceFileToTempDir(bool enabled) noexcept;
  bool IsSaveSliceFileToTempDir() const noexcept;

  Result SetThreadNum(size_t thread_num) noexcept;
  size_t GetThreadNum() const noexcept;

  utf8string GetUrl() const noexcept;
  utf8string GetTargetFilePath() const noexcept;

  Result SetNetworkConnectionTimeout(size_t milliseconds) noexcept;  // default is 3000ms
  size_t GetNetworkConnectionTimeout() const noexcept;

  Result SetNetworkReadTimeout(size_t milliseconds) noexcept;  // default is 3000ms
  size_t GetNetworkReadTimeout() const noexcept;

  Result SetQueryFileSizeRetryTimes(size_t retry_times) noexcept;
  size_t GetQueryFileSizeRetryTimes() const noexcept;

  // default is -1 = forever, 0 = not use exist slice cache
  void SetSliceExpiredTime(int seconds) noexcept;
  int GetSliceExpiredTime() const noexcept;

  void SetMaxDownloadSpeed(size_t byte_per_seconds) noexcept;  // default is 0 = not limit
  size_t GetMaxDownloadSpeed() const noexcept;

  void SetDiskCacheSize(size_t cache_size) noexcept;  // byte
  size_t GetDiskCacheSize() const noexcept; // byte

  std::shared_future<Result> Start(const utf8string url,
                           const utf8string& target_file_path,
                           ResultFunctor result_functor,
                           ProgressFunctor progress_functor,
                           RealtimeSpeedFunctor realtime_speed_functor,
                           CancelEvent* cancel_event = nullptr) noexcept;

  void Stop(bool wait = false) noexcept;

 protected:
  class TeemoImpl;
  TeemoImpl* impl_;

  Teemo(const Teemo&) = delete;
  Teemo& operator=(const Teemo&) = delete;
};
}  // namespace teemo

#endif