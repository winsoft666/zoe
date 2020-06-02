#pragma once

#define ONLY_BASIC_STRCUT
#include "teemo/teemo.h"

namespace teemo {
  typedef struct _Options {
    bool can_update_url;
    int32_t thread_num;
    int32_t disk_cache_size;
    int32_t max_speed;
    int32_t tmp_file_expired_time;
    int32_t fetch_file_info_retry;
    int32_t network_conn_timeout;
    int32_t network_read_timeout;
    int64_t fixed_slice_size;

    ResultFunctor result_functor;
    ProgressFunctor progress_functor;
    RealtimeSpeedFunctor speed_functor;
    VerboseOuputFunctor verbose_functor;

    utf8string url;
    utf8string target_file_path;

    _Options() {
      can_update_url = false;
      thread_num = 1;
      disk_cache_size = 20 * (2 << 19);
      fixed_slice_size = 10 * (2 << 19);
      max_speed = -1;
      tmp_file_expired_time = -1;
      fetch_file_info_retry = 1;
      network_conn_timeout = 3000;
      network_read_timeout = 3000;

      result_functor = nullptr;
      progress_functor = nullptr;
      speed_functor = nullptr;
      verbose_functor = nullptr;
    }
  } Options;
}