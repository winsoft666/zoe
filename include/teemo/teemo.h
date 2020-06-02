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
  SUCCESSED = 0,
  UNKNOWN_ERROR,
  INVALID_URL,
  INVALID_INDEX_FORMAT,
  INVALID_TARGET_FILE_PATH,
  INVALID_THREAD_NUM,
  INVALID_NETWORK_CONN_TIMEOUT,
  INVALID_NETWORK_READ_TIMEOUT,
  INVALID_FETCH_FILE_INFO_RETRY_TIMES,
  ALREADY_DOWNLOADING,
  CANCELED,
  RENAME_TMP_FILE_FAILED,
  OPEN_INDEX_FILE_FAILED,
  TMP_FILE_EXPIRED,
  INIT_CURL_FAILED,
  INIT_CURL_MULTI_FAILED,
  SET_CURL_OPTION_FAILED,
  ADD_CURL_HANDLE_FAILED,
  CREATE_TARGET_FILE_FAILED,
  CREATE_TMP_FILE_FAILED,
  OPEN_TMP_FILE_FAILED,
  URL_DIFFERENT,
  TMP_FILE_SIZE_ERROR,
  TMP_FILE_CANNOT_RW,
  FLUSH_TMP_FILE_FAILED,
  UPDATE_INDEX_FILE_FAILED,
  SLICE_DOWNLOAD_FAILED
};


TEEMO_API const char* GetResultString(int enumVal);

class TEEMO_API Event {
 public:
  Event(bool setted = false);
  ~Event();

  void set() noexcept;
  void unSet() noexcept;
  bool isSetted() noexcept;
  bool wait(int32_t millseconds) noexcept;

 protected:
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;
  bool setted_;
  std::mutex setted_mutex_;
  std::condition_variable setted_cond_var_;
};

typedef std::string utf8string;
typedef std::function<void(Result ret)> ResultFunctor;
typedef std::function<void(int64_t total, int64_t downloaded)> ProgressFunctor;
typedef std::function<void(int64_t byte_per_sec)> RealtimeSpeedFunctor;
typedef std::function<void(const utf8string& verbose)> VerboseOuputFunctor;

class TEEMO_API Teemo {
 public:
  Teemo();
  ~Teemo();

  static void GlobalInit();
  static void GlobalUnInit();

  void setVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept;

  Result setThreadNum(int32_t thread_num) noexcept;
  int32_t threadNum() const noexcept;

  utf8string url() const noexcept;
  utf8string targetFilePath() const noexcept;

  Result setNetworkConnectionTimeout(int32_t milliseconds) noexcept;  // default is 3000ms
  int32_t networkConnectionTimeout() const noexcept;

  Result setNetworkReadTimeout(int32_t milliseconds) noexcept;  // default is 3000ms
  int32_t networkReadTimeout() const noexcept;

  Result setFetchFileInfoRetryTimes(int32_t retry_times) noexcept;
  int32_t fetchFileInfoRetryTimes() const noexcept;

  // default is -1 = forever, 0 = not use exist slice cache
  void setTmpFileExpiredTime(int32_t seconds) noexcept;
  int32_t tmpFileExpiredTime() const noexcept;

  // default is -1 = not limit
  void setMaxDownloadSpeed(int32_t byte_per_seconds) noexcept;
  int32_t maxDownloadSpeed() const noexcept;

  // default is 20Mb
  Result setDiskCacheSize(int32_t cache_size) noexcept;  // byte
  int32_t diskCacheSize() const noexcept;             // byte

  std::shared_future<Result> start(const utf8string& url,
                                   const utf8string& target_file_path,
                                   ResultFunctor result_functor,
                                   ProgressFunctor progress_functor,
                                   RealtimeSpeedFunctor realtime_speed_functor,
                                   bool can_update_url = false) noexcept;

  void stop(bool wait = false) noexcept;

 protected:
  class TeemoImpl;
  TeemoImpl* impl_;

  Teemo(const Teemo&) = delete;
  Teemo& operator=(const Teemo&) = delete;
};
}  // namespace teemo

#endif