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
#ifndef TEEMO_H_
#define TEEMO_H_
#pragma once

#include <string>
#include <memory>
#include <future>
#include <map>
#include "config.h"

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

namespace TEEMO_NAMESPACE {
enum Result {
  SUCCESSED = 0,
  UNKNOWN_ERROR = 1,
  INVALID_URL = 2,
  INVALID_INDEX_FORMAT = 3,
  INVALID_TARGET_FILE_PATH = 4,
  INVALID_THREAD_NUM = 5,
  INVALID_HASH_POLICY = 6,
  INVALID_SLICE_POLICY = 7,
  INVALID_NETWORK_CONN_TIMEOUT = 8,
  INVALID_NETWORK_READ_TIMEOUT = 9,
  INVALID_FETCH_FILE_INFO_RETRY_TIMES = 10,
  ALREADY_DOWNLOADING = 11,
  CANCELED = 12,
  RENAME_TMP_FILE_FAILED = 13,
  OPEN_INDEX_FILE_FAILED = 14,
  TMP_FILE_EXPIRED = 15,
  INIT_CURL_FAILED = 16,
  INIT_CURL_MULTI_FAILED = 17,
  SET_CURL_OPTION_FAILED = 18,
  ADD_CURL_HANDLE_FAILED = 19,
  CREATE_TARGET_FILE_FAILED = 20,
  CREATE_TMP_FILE_FAILED = 21,
  OPEN_TMP_FILE_FAILED = 22,
  URL_DIFFERENT = 23,
  TMP_FILE_SIZE_ERROR = 24,
  TMP_FILE_CANNOT_RW = 25,
  FLUSH_TMP_FILE_FAILED = 26,
  UPDATE_INDEX_FILE_FAILED = 27,
  SLICE_DOWNLOAD_FAILED = 28,
  HASH_VERIFY_NOT_PASS = 29,
  CALCULATE_HASH_FAILED = 30,
  FETCH_FILE_INFO_FAILED = 31,
  REDIRECT_URL_DIFFERENT = 32,
  NOT_CLEARLY_RESULT = 33,
};

enum DownloadState { STOPPED = 0, DOWNLODING = 1, PAUSED = 2 };

TEEMO_API const char* GetResultString(int enumVal);

enum SlicePolicy { Auto = 0, FixedSize, FixedNum };

enum HashType { MD5 = 0, CRC32, SHA1, SHA256 };

enum HashVerifyPolicy { ALWAYS = 0, ONLY_NO_FILESIZE };

enum UncompletedSliceSavePolicy { ALWAYS_DISCARD = 0, SAVE_EXCEPT_FAILED };

class TEEMO_API Event {
 public:
  Event(bool setted = false);
  ~Event();

  void set() noexcept;
  void unset() noexcept;
  bool isSetted() noexcept;
  bool wait(int32_t millseconds) noexcept;

 protected:
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;
  class EventImpl;
  EventImpl* impl_;
};

typedef std::string utf8string;
typedef std::function<void(Result ret)> ResultFunctor;
typedef std::function<void(int64_t total, int64_t downloaded)> ProgressFunctor;
typedef std::function<void(int64_t byte_per_sec)> RealtimeSpeedFunctor;
typedef std::function<void(const utf8string& verbose)> VerboseOuputFunctor;
typedef std::multimap<utf8string, utf8string> HttpHeaders;

class TEEMO_API TEEMO {
 public:
  TEEMO();
  ~TEEMO();

  static void GlobalInit();
  static void GlobalUnInit();

  void setVerboseOutput(VerboseOuputFunctor verbose_functor) noexcept;

  // Pass an int specifying the maximum thread number.
  // teemo will use these threads as much as possible.
  // Set to 0 or negative to switch to the default built-in thread number - 1.
  // The number of threads cannot be greater than 100, otherwise teemo will return INVALID_THREAD_NUM.
  //
  Result setThreadNum(int32_t thread_num) noexcept;
  int32_t threadNum() const noexcept;

  // Pass an int. It should contain the maximum time in milliseconds that you allow the connection phase to the server to take.
  // This only limits the connection phase, it has no impact once it has connected.
  // Set to 0 or negative to switch to the default built-in connection timeout - 3000 milliseconds.
  //
  Result setNetworkConnectionTimeout(
      int32_t milliseconds) noexcept;                 // milliseconds
  int32_t networkConnectionTimeout() const noexcept;  // milliseconds

  // Pass an int specifying the retry times when request file information(such as file size) failed.
  // Set to 0 or negative to switch to the default built-in retry times - 1.
  //
  Result setFetchFileInfoRetryTimes(int32_t retry_times) noexcept;
  int32_t fetchFileInfoRetryTimes() const noexcept;

  // If bUseHead is true, teemo will use HEAD method to fetch file info. Otherwise, teemo will use GET method.
  Result setFetchFileInfoHeadMethod(bool use_head) noexcept;
  bool fetchFileInfoHeadMethod() const noexcept;

  // Pass an int as parameter.
  // If the interval seconds that from the saved time of temporary file to present greater than or equal to this parameter, the temporary file will be discarded.
  // Default to -1, never expired.
  //
  Result setTmpFileExpiredTime(int32_t seconds) noexcept;  // seconds
  int32_t tmpFileExpiredTime() const noexcept;             // seconds

  // Pass an int as parameter.
  // If a download exceeds this speed (counted in bytes per second) the transfer will pause to keep the speed less than or equal to the parameter value.
  // Defaults to -1, unlimited speed.
  // Set to 0 or negative to switch to the default built-in limit - -1(unlimited speed).
  // This option doesn't affect transfer speeds done with FILE:// URLs.
  //
  Result setMaxDownloadSpeed(int32_t byte_per_seconds) noexcept;
  int32_t maxDownloadSpeed() const noexcept;

  // Pass an int as parameter.
  // If a download less than this speed (counted in bytes per second) during "low speed time" seconds,
  // the transfer will be considered as failed.
  // Default to -1, unlimited speed.
  // Set to 0 or negative to switch to the default built-in limit - -1(unlimited speed).
  //
  Result setMinDownloadSpeed(int32_t byte_per_seconds,
                             int32_t duration) noexcept;  // seconds
  int32_t minDownloadSpeed() const noexcept;
  int32_t minDownloadSpeedDuration() const noexcept;  // seconds

  // Pass an unsigned int specifying your maximal size for the disk cache total buffer in teemo.
  // This buffer size is by default 20971520 byte (20MB).
  //
  Result setDiskCacheSize(int32_t cache_size) noexcept;  // byte
  int32_t diskCacheSize() const noexcept;                // byte

  // Set an event, teemo will stop downloading when this event set.
  // If download is stopped for stop_event set or call stop, teemo will return CANCELED.
  //
  Result setStopEvent(Event* stop_event) noexcept;
  Event* stopEvent() noexcept;

  // Set false, teemo will not check whether the redirected url is the same as in the index file,
  // Default to true, if the redirected url is different from the url in the index file, teemo will return REDIRECT_URL_DIFFERENT error.
  //
  Result setRedirectedUrlCheckEnabled(bool enabled) noexcept;
  bool redirectedUrlCheckEnabled() const noexcept;

  // Set true, teemo will parse Content-Md5 header filed and make sure target file's md5 is same as this value,
  // and in this case, slice files will be expired if content_md5 value that cached in index file is changed.
  // Content-Md5 is pure md5 string, not by base64.
  // Default to false.
  //
  Result setContentMd5Enabled(bool enabled) noexcept;
  bool contentMd5Enabled() const noexcept;

  // Set slice policy, tell teemo how to calculate each slice size.
  // Default: fixed to 10485760 bytes(10MB)
  //
  Result setSlicePolicy(SlicePolicy policy, int64_t policy_value) noexcept;
  void slicePolicy(SlicePolicy& policy, int64_t& policy_value) const noexcept;

  // Set hash verify policy, the hash value is the whole file's hash, not for a slice.
  // If fetch file size failed, hash verify is the only way to know whether file download completed.
  // If hash value is empty, will not calculate hash, nor verify hash value.
  //
  Result setHashVerifyPolicy(HashVerifyPolicy policy,
                             HashType hash_type,
                             const utf8string& hash_value) noexcept;
  void hashVerifyPolicy(HashVerifyPolicy& policy,
                        HashType& hash_type,
                        utf8string& hash_value) const noexcept;

  Result setHttpHeaders(const HttpHeaders& headers) noexcept;
  HttpHeaders httpHeaders() const noexcept;

  // set proxy string, such as http://127.0.0.1:8888
  //
  Result setProxy(const utf8string& proxy) noexcept;
  utf8string proxy() const noexcept;

  // Set uncompleted slice save policy.
  // Default is ALWAYS_DISCARD, because teemo doesn't know how to check slice(especially uncompleted) is valid or not.
  //
  Result setUncompletedSliceSavePolicy(
      UncompletedSliceSavePolicy policy) noexcept;
  UncompletedSliceSavePolicy uncompletedSliceSavePolicy() const noexcept;

  // Start to download and state change to DOWNLOADING.
  // Supported url protocol is the same as libcurl.
  //
  std::shared_future<Result> start(
      const utf8string& url,
      const utf8string& target_file_path,
      ResultFunctor result_functor,
      ProgressFunctor progress_functor,
      RealtimeSpeedFunctor realtime_speed_functor) noexcept;

  // Pause downloading and state change to PAUSED.
  //
  void pause() noexcept;

  // Resume downloading and state change to DOWNLOADING.
  void resume() noexcept;

  // Stop downloading and state change to STOPPED, teemo will return CANCELED in ResultFunctor.
  //
  void stop() noexcept;

  utf8string url() const noexcept;
  utf8string targetFilePath() const noexcept;

  // The file size of server side that will be downloaded.
  // Set to (-1) when get original file size failed.
  //
  int64_t originFileSize() const noexcept;

  DownloadState state() const noexcept;

  std::shared_future<Result> futureResult() noexcept;

 protected:
  class TeemoImpl;
  TeemoImpl* impl_;

  TEEMO(const TEEMO&) = delete;
  TEEMO& operator=(const TEEMO&) = delete;
};
}  // namespace teemo
#endif  // !TEEMO_H_