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

#ifndef TEEMO_OPTIONS_H_
#define TEEMO_OPTIONS_H_
#pragma once

#include "teemo/teemo.h"

namespace teemo {
#define TEEMO_DEFAULT_NETWORK_CONN_TIMEOUT_MS 3000
#define TEEMO_DEFAULT_TOTAL_DISK_CACHE_SIZE_BYTE 20971520  // 20MB
#define TEEMO_DEFAULT_FIXED_SLICE_SIZE_BYTE 10485760       // 10MB
#define TEEMO_DEFAULT_FIXED_SLICE_NUM 1
#define TEEMO_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES 1
#define TEEMO_DEFAULT_THREAD_NUM 1

typedef struct _Options {
  bool skipping_url_check;
  int32_t thread_num;
  int32_t disk_cache_size;
  int32_t max_speed;
  int32_t tmp_file_expired_time;
  int32_t fetch_file_info_retry;
  int32_t network_conn_timeout;

  SlicePolicy slice_policy;
  int64_t slice_policy_value;

  HashVerifyPolicy hash_verify_policy;
  HashType hash_type;
  utf8string hash_value;

  ResultFunctor result_functor;
  ProgressFunctor progress_functor;
  RealtimeSpeedFunctor speed_functor;
  VerboseOuputFunctor verbose_functor;

  mutable Event internal_stop_event;
  Event* user_stop_event;

  utf8string url;
  utf8string target_file_path;

  _Options() : internal_stop_event(true) {
    skipping_url_check = false;
    thread_num = TEEMO_DEFAULT_THREAD_NUM;
    disk_cache_size = TEEMO_DEFAULT_TOTAL_DISK_CACHE_SIZE_BYTE;

    slice_policy = Auto;
    slice_policy_value = 0L;

    hash_verify_policy = ALWAYS;
    hash_type = MD5;

    max_speed = -1;
    tmp_file_expired_time = -1;
    fetch_file_info_retry = TEEMO_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES;
    network_conn_timeout = TEEMO_DEFAULT_NETWORK_CONN_TIMEOUT_MS;

    result_functor = nullptr;
    progress_functor = nullptr;
    speed_functor = nullptr;
    verbose_functor = nullptr;

    user_stop_event = nullptr;
  }
} Options;
}  // namespace teemo
#endif  // !TEEMO_OPTIONS_H_