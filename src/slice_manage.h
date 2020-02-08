/*******************************************************************************
* Copyright (C) 2018 - 2020, winsoft666, <winsoft666@outlook.com>.
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

#ifndef TEEMO_SLICE_MANAGE_H__
#define TEEMO_SLICE_MANAGE_H__
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <future>
#include <condition_variable>
#include "slice.h"
#include "curl/curl.h"
#include "teemo/teemo.h"

namespace teemo {
class Slice;
class SliceManage : public std::enable_shared_from_this<SliceManage> {
 public:
  SliceManage();
  virtual ~SliceManage();

  void SetVerboseOutput(VerboseOuputFunctor verbose_functor);

  Result SetNetworkConnectionTimeout(size_t conn_timeout_ms);
  size_t GetNetworkConnectionTimeout() const;

  Result SetNetworkReadTimeout(size_t read_timeout_ms);
  size_t GetNetworkReadTimeout() const;

  Result SetQueryFileSizeRetryTimes(size_t retry_times);
  size_t GetQueryFileSizeRetryTimes() const;

  void SetSliceCacheExpiredTime(int seconds);
  int GetSliceCacheExpiredTime() const;

  Result SetThreadNum(size_t thread_num);
  size_t GetThreadNum() const;

  void SetSaveSliceFileToTempDir(bool enabled);
  bool IsSaveSliceFileToTempDir() const;

  void SetMaxDownloadSpeed(size_t byte_per_seconds);
  size_t GetMaxDownloadSpeed() const;

  Result Start(const utf8string& url,
               const utf8string& target_file_path,
               ProgressFunctor progress_functor,
               RealtimeSpeedFunctor realtime_speed_functor,
               const Concurrency::cancellation_token_source& cancel_token);
  void Stop();

  utf8string GetUrl() const;
  utf8string GetTargetFilePath() const;
  utf8string GetIndexFilePath() const;
  void OutputVerboseInfo(const utf8string& info);

 protected:
  long QueryFileSize() const;
  bool LoadSlices(const utf8string url, ProgressFunctor functor);
  bool CombineSlice();
  bool CleanupTmpFiles();
  bool UpdateIndexFile();
  void Destory();
  Result GenerateIndexFilePath(const utf8string& target_file_path, utf8string& index_path) const;

 protected:
  utf8string url_;
  utf8string target_file_path_;
  utf8string index_file_path_;
  bool save_slice_to_tmp_dir_;
  size_t thread_num_;
  size_t network_conn_timeout_;
  size_t network_read_timeout_;
  size_t max_download_speed_;
  size_t query_filesize_retry_times_;
  int slice_cache_expired_seconds_;
  long file_size_;
  CURLM* multi_;
  ProgressFunctor progress_functor_;
  RealtimeSpeedFunctor speed_functor_;
  VerboseOuputFunctor verbose_functor_;
  Concurrency::cancellation_token_source cancel_token_;
  std::vector<std::shared_ptr<Slice>> slices_;
  std::future<void> progress_notify_thread_;
  std::future<void> speed_notify_thread_;

  bool stop_;
  std::mutex stop_mutex_;
  std::condition_variable stop_cond_var_;
};
}  // namespace teemo

#endif