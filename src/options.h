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

#ifndef ZOE_OPTIONS_H_
#define ZOE_OPTIONS_H_
#pragma once

#include "zoe/zoe.h"

namespace zoe {
#define ZOE_DEFAULT_NETWORK_CONN_TIMEOUT_MS 3000
#define ZOE_DEFAULT_TOTAL_DISK_CACHE_SIZE_BYTE 20971520  // 20MB
#define ZOE_DEFAULT_FIXED_SLICE_SIZE_BYTE 10485760  // 10MB
#define ZOE_DEFAULT_FIXED_SLICE_NUM 1
#define ZOE_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES 1
#define ZOE_DEFAULT_THREAD_NUM 1
#define ZOE_DEFAULT_SLICE_MAX_FAILED_TIMES 3

typedef struct _Options {
  bool redirected_url_check_enabled;
  bool content_md5_enabled;
  bool use_head_method_fetch_file_info;
  int32_t thread_num;
  int32_t disk_cache_size;
  int32_t max_speed;
  int32_t min_speed;
  int32_t min_speed_duration;
  int32_t tmp_file_expired_time;
  int32_t fetch_file_info_retry;
  int32_t network_conn_timeout;

  int32_t slice_max_failed_times;

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

  HttpHeaders http_headers;

  utf8string proxy;

  UncompletedSliceSavePolicy uncompleted_slice_save_policy;

  _Options() : internal_stop_event(true) {
    redirected_url_check_enabled = true;
    content_md5_enabled = false;
    use_head_method_fetch_file_info = true;
    thread_num = ZOE_DEFAULT_THREAD_NUM;
    disk_cache_size = ZOE_DEFAULT_TOTAL_DISK_CACHE_SIZE_BYTE;

    slice_policy = Auto;
    slice_policy_value = 0L;

    hash_verify_policy = ALWAYS;
    hash_type = MD5;

    max_speed = -1;
    min_speed = -1;
    min_speed_duration = 0;
    tmp_file_expired_time = -1;
    fetch_file_info_retry = ZOE_DEFAULT_FETCH_FILE_INFO_RETRY_TIMES;
    network_conn_timeout = ZOE_DEFAULT_NETWORK_CONN_TIMEOUT_MS;

    slice_max_failed_times = ZOE_DEFAULT_SLICE_MAX_FAILED_TIMES;

    result_functor = nullptr;
    progress_functor = nullptr;
    speed_functor = nullptr;
    verbose_functor = nullptr;

    user_stop_event = nullptr;

    uncompleted_slice_save_policy = ALWAYS_DISCARD;
  }
} Options;
}  // namespace zoe
#endif  // !ZOE_OPTIONS_H_