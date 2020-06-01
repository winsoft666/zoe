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

#include "slice_manage.h"
#include <array>
#include <algorithm>
#include <sstream>
#include <iostream>
#include "json.hpp"
#include "file_util.h"
#include "curl_utils.h"

using json = nlohmann::json;

#define INDEX_FILE_SIGN_STRING "TEEMO:EASY-FILE-DOWNLOAD(2.0)"

namespace teemo {

utf8string bool_to_string(bool b) {
  return (b ? "true" : "false");
}

SliceManage::SliceManage()
    : multi_(nullptr)
    , thread_num_(1)
    , slice_expired_seconds_(-1)
    , network_conn_timeout_(3000)
    , network_read_timeout_(3000)
    , max_download_speed_(0)
    , query_filesize_retry_times_(3)
    , stop_(true)
    , verbose_functor_(nullptr)
    , speed_functor_(nullptr)
    , progress_functor_(nullptr)
    , cancel_event_(nullptr)
    , disk_cache_total_size_(20 * (2 << (20 - 1)))
    , file_size_(-1) {
  target_file_ = std::make_shared<TargetFile>();
}

SliceManage::~SliceManage() {
  target_file_.reset();
}

void SliceManage::SetVerboseOutput(VerboseOuputFunctor verbose_functor) {
  verbose_functor_ = verbose_functor;
}

Result SliceManage::SetNetworkConnectionTimeout(size_t conn_timeout_ms) {
  if (!stop_)
    return AlreadyDownloading;
  if (conn_timeout_ms == 0)
    return NetworkConnTimeoutInvalid;

  network_conn_timeout_ = conn_timeout_ms;
  return Successed;
}

size_t SliceManage::GetNetworkConnectionTimeout() const {
  return network_conn_timeout_;
}

Result SliceManage::SetNetworkReadTimeout(size_t read_timeout_ms) {
  if (!stop_)
    return AlreadyDownloading;
  if (read_timeout_ms == 0)
    return NetworkReadTimeoutInvalid;

  network_read_timeout_ = read_timeout_ms;
  return Successed;
}

size_t SliceManage::GetNetworkReadTimeout() const {
  return network_read_timeout_;
}

Result SliceManage::SetQueryFileSizeRetryTimes(size_t retry_times) {
  if (!stop_)
    return AlreadyDownloading;
  if (retry_times == 0)
    return QueryFileSizeRetryTimesInvalid;

  query_filesize_retry_times_ = retry_times;
  return Successed;
}

size_t SliceManage::GetQueryFileSizeRetryTimes() const {
  return query_filesize_retry_times_;
}

void SliceManage::SetSliceExpiredTime(int seconds) {
  slice_expired_seconds_ = seconds;
}

int SliceManage::GetSliceExpiredTime() const {
  return slice_expired_seconds_;
}

Result SliceManage::SetThreadNum(size_t thread_num) {
  if (!stop_)
    return AlreadyDownloading;
  if (thread_num == 0 || thread_num > 100)
    return ThreadNumInvalid;

  thread_num_ = thread_num;
  return Successed;
}

size_t SliceManage::GetThreadNum() const {
  return thread_num_;
}

void SliceManage::SetMaxDownloadSpeed(size_t byte_per_seconds) {
  max_download_speed_ = byte_per_seconds;
}

size_t SliceManage::GetMaxDownloadSpeed() const {
  return max_download_speed_;
}

void SliceManage::SetDiskCacheSize(size_t cache_size) noexcept {
  disk_cache_total_size_ = cache_size;
}

size_t SliceManage::GetDiskCacheSize() const noexcept {
  return disk_cache_total_size_;
}

Result SliceManage::Start(const utf8string& url,
                          const utf8string& target_file_path,
                          ProgressFunctor progress_functor,
                          RealtimeSpeedFunctor realtime_speed_functor,
                          CancelEvent* cancel_event) {
  if (url.length() == 0)
    return Result::UrlInvalid;

  // be sure previous threads has exit
  if (progress_notify_thread_.valid())
    progress_notify_thread_.wait();
  if (speed_notify_thread_.valid())
    speed_notify_thread_.wait();

  if (target_file_->IsOpened())
    target_file_->Close();

  stop_ = false;

  url_ = url;
  cancel_event_ = cancel_event;
  progress_functor_ = progress_functor;
  speed_functor_ = realtime_speed_functor;
  target_file_path_ = target_file_path;
  Result r = GenerateIndexFilePath(target_file_path, index_file_path_);
  if (r != Successed) {
    return r;
  }

  if (verbose_functor_) {
    OutputVerboseInfo("max download speed: " + std::to_string(max_download_speed_) + "\r\n");
    OutputVerboseInfo("network conn timeout: " + std::to_string(network_conn_timeout_) + "\r\n");
    OutputVerboseInfo("network read timeout: " + std::to_string(network_read_timeout_) + "\r\n");
    OutputVerboseInfo("slice expired seconds: " + std::to_string(slice_expired_seconds_) + "\r\n");
    OutputVerboseInfo("target file path: " + target_file_path_ + "\r\n");
    OutputVerboseInfo("index file path: " + index_file_path_ + "\r\n");
  }

  bool valid_resume = false;
  do {
    bool bret = FileIsRW(index_file_path_);
    OutputVerboseInfo("index file path RW: " + bool_to_string(bret) + "\r\n");
    if (!bret)
      break;

    bret = LoadSlices(index_file_path_, url_, progress_functor);
    OutputVerboseInfo("load slices: " + bool_to_string(bret) + "\r\n");
    if (!bret) {
      bool remove_ret = RemoveFile(index_file_path_);
      OutputVerboseInfo("remove index file: " + bool_to_string(remove_ret) + "\r\n");
      break;
    }
    if (file_size_ == -1)
      break;
    if (slices_.size() == 0)
      break;
    valid_resume = true;
  } while (false);

  if (verbose_functor_) {
    OutputVerboseInfo("valid resume download: " + bool_to_string(valid_resume) + "\r\n");
  }

  if (cancel_event_ && cancel_event_->IsCanceled()) {
    Destory();
    return Canceled;
  }

  if (!valid_resume) {
    size_t try_times = 0;
    do {
      file_size_ = QueryFileSize();
      if (cancel_event_ && cancel_event_->IsCanceled()) {
        Destory();
        return Canceled;
      }

      if (file_size_ != -1)
        break;
    } while (++try_times < query_filesize_retry_times_);

    if (verbose_functor_) {
      OutputVerboseInfo("queried file size: " + std::to_string(file_size_) + "\r\n");
    }
    slices_.clear();

    target_tmp_file_path_ = GenerateTmpFilePath(target_file_path_);
    if (!target_file_->Create(target_tmp_file_path_, file_size_ < 0 ? 0 : file_size_)) {
      return CreateTmpFileFailed;
    }

    if (file_size_ > 0) {
      thread_num_ = std::min(file_size_, (long)thread_num_);
      if (verbose_functor_) {
        OutputVerboseInfo("thread num: " + std::to_string(thread_num_) + "\r\n");
      }

      long each_slice_size = 0L;
      long each_slice_disk_cache_size = 0L;

      if (thread_num_ > 0) {
        each_slice_size = file_size_ / thread_num_;
        each_slice_disk_cache_size = disk_cache_total_size_ / thread_num_;
      }
      if (verbose_functor_) {
        OutputVerboseInfo("each slice size: " + std::to_string(each_slice_size) + "\r\n");
        OutputVerboseInfo(
            "each slice disk cache size: " + std::to_string(each_slice_disk_cache_size) + "\r\n");
      }

      for (size_t i = 0; i < thread_num_; i++) {
        long end = (i == thread_num_ - 1) ? file_size_ - 1 : ((i + 1) * each_slice_size) - 1;
        std::shared_ptr<Slice> slice = std::make_shared<Slice>(i, shared_from_this());
        Result r =
            slice->Init(target_file_, i * each_slice_size, end, 0, each_slice_disk_cache_size);
        if (r != Successed) {
          Destory();
          return r;
        }
        slices_.push_back(slice);
      }
    }
    else if (file_size_ == 0) {
      FILE* f = OpenFile(target_file_path_, u8"wb");
      if (!f)
        return GenerateTargetFileFailed;
      fclose(f);
      Destory();
      return Successed;
    }
    else {
      thread_num_ = 1;
      if (verbose_functor_) {
        OutputVerboseInfo("thread num: " + std::to_string(thread_num_) + "\r\n");
      }

      std::shared_ptr<Slice> slice = std::make_shared<Slice>(0, shared_from_this());
      Result r = slice->Init(target_file_, 0, -1, 0, disk_cache_total_size_);
      if (r != Successed) {
        Destory();
        return r;
      }
      slices_.push_back(slice);
    }
  }

  if (verbose_functor_) {
    std::stringstream ss_verbose;
    ss_verbose << "slices: \r\n";
    for (auto& s : slices_) {
      ss_verbose << s->index() << ": " << s->begin() << " ~ " << s->end() << ": " << s->capacity()
                 << "\r\n";
    }
    OutputVerboseInfo(ss_verbose.str());
  }

  if (cancel_event_ && cancel_event_->IsCanceled()) {
    Destory();
    return Canceled;
  }

  size_t uncomplete_slice_num = 0;
  for (auto& slice : slices_) {
    if (!slice->IsDownloadCompleted())
      uncomplete_slice_num++;
  }
  if (verbose_functor_) {
    OutputVerboseInfo("uncompleted slice num: " + std::to_string(uncomplete_slice_num) + "\r\n");
  }

  if (uncomplete_slice_num == 0) {
    Result ret;
    do {
      if (!RenameTargetFile(target_tmp_file_path_, target_file_path_, true)) {
        if (verbose_functor_) {
          OutputVerboseInfo("rename target file failed\r\n");
        }
        ret = RenameTargetFileFailed;
        break;
      }

      if (!RemoveFile(index_file_path_)) {
        if (verbose_functor_) {
          OutputVerboseInfo("remove index file failed\r\n");
        }
      }
      ret = Successed;
    } while (false);

    Destory();
    if (progress_functor_)
      progress_functor_(file_size_, file_size_);
    return ret;
  }

  multi_ = curl_multi_init();
  if (!multi_) {
    OutputVerboseInfo("curl_multi_init failed\r\n");
    Destory();
    return Result::InternalNetworkError;
  }

  thread_num_ = uncomplete_slice_num;
  if (verbose_functor_) {
    OutputVerboseInfo("thread num: " + std::to_string(thread_num_) + "\r\n");
  }

  size_t each_slice_download_speed = max_download_speed_ / uncomplete_slice_num;
  if (verbose_functor_) {
    OutputVerboseInfo(
        "each slice max download speed: " + std::to_string(each_slice_download_speed) + "\r\n");
  }

  bool init_curl_ret = true;
  for (auto& slice : slices_) {
    if (!slice->IsDownloadCompleted()) {
      if (!slice->InitCURL(multi_, each_slice_download_speed)) {
        init_curl_ret = false;
        break;
      }
    }
  }

  if (!init_curl_ret) {
    if (verbose_functor_) {
      OutputVerboseInfo("init curl failed\r\n");
    }
    Destory();
    return Result::InternalNetworkError;
  }

  long init_total_capacity = 0;
  for (const auto& s : slices_)
    init_total_capacity += s->capacity();

  if (verbose_functor_) {
    OutputVerboseInfo("init total capacity: " + std::to_string(init_total_capacity) + "\r\n");
  }

  if (progress_functor_) {
    progress_notify_thread_ =
        std::async(std::launch::async, std::bind(&SliceManage::ProgressNotifyThreadProc, this));
  }

  if (speed_functor_) {
    speed_notify_thread_ =
        std::async(std::launch::async,
                   std::bind(&SliceManage::SpeedNotifyThreadProc, this, init_total_capacity));
  }

  int still_running = 0;
  CURLMcode m_code = curl_multi_perform(multi_, &still_running);
  size_t thread_num_copy = thread_num_;

  do {
    {
      std::lock_guard<std::mutex> lg(stop_mutex_);
      if (stop_)
        break;
    }

    if (cancel_event_ && cancel_event_->IsCanceled()) {
      stop_ = true;
      break;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    long curl_timeo = -1;
    curl_multi_timeout(multi_, &curl_timeo);

    if (curl_timeo > 0) {
      timeout.tv_sec = curl_timeo / 1000;
      if (timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
    }
    else {
      timeout.tv_sec = 0;
      timeout.tv_usec = 100 * 1000;
    }

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    CURLMcode code = curl_multi_fdset(multi_, &fdread, &fdwrite, &fdexcep, &maxfd);
    if (code != CURLM_CALL_MULTI_PERFORM && code != CURLM_OK) {
      if (verbose_functor_) {
        OutputVerboseInfo("\r\ncurl_multi_fdset failed, code: " + std::to_string((int)code) +
                          "\r\n");
      }
      break;
    }

    /* On success the value of maxfd is guaranteed to be >= -1. We call
      select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
      no fds ready yet so we sleep 100ms, which is the minimum suggested value in the
      curl_multi_fdset() doc. 
    */
    int rc;
    if (maxfd == -1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      rc = 0;
    }
    else {
      rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    }

    switch (rc) {
      case -1:
        break; /* select error */
      case 0:  /* timeout */
      default: /* action */
        curl_multi_perform(multi_, &still_running);
        break;
    }

  } while (still_running);

  if (verbose_functor_) {
    OutputVerboseInfo("\r\nstill running handles: " + std::to_string(still_running) + "\r\n");
  }

  /* See how the transfers went */
  size_t done_thread = 0;
  CURLMsg* msg = NULL;
  int msgsInQueue;
  while ((msg = curl_multi_info_read(multi_, &msgsInQueue)) != NULL) {
    if (msg->msg == CURLMSG_DONE) {
      if (msg->data.result == CURLE_OK) {
        done_thread++;
      }
    }
  }

  if (verbose_functor_) {
    OutputVerboseInfo("done thread num: " + std::to_string(done_thread) + "\r\n");
    OutputVerboseInfo("stop: " + bool_to_string(stop_) + "\r\n");
  }

  long total_capacity = 0;
  do {
    std::lock_guard<std::recursive_mutex> lg(slices_mutex_);
    for (auto slice : slices_) {
      if (!slice->FlushDiskCache()) {
        // TODO
      }
      total_capacity += slice->capacity();
    }
  } while (false);

  target_file_->Close();

  if (verbose_functor_) {
    OutputVerboseInfo("total capacity: " + std::to_string(total_capacity) + "\r\n");
  }

  Result ret;
  do {
    if (done_thread == thread_num_copy) {
      if (file_size_ == -1 || (file_size_ > 0 && total_capacity == file_size_)) {
        if (!RenameTargetFile(target_tmp_file_path_, target_file_path_, true)) {
          if (verbose_functor_) {
            OutputVerboseInfo("rename target file failed\r\n");
          }
          ret = RenameTargetFileFailed;
          break;
        }

        if (!RemoveFile(index_file_path_)) {
          if (verbose_functor_) {
            OutputVerboseInfo("remove index file failed\r\n");
          }
        }

        if (progress_functor_)
          progress_functor_(file_size_, total_capacity);

        ret = Successed;
        break;
      }
    }

    if (stop_) {
      if (UpdateIndexFile(index_file_path_)) {
        ret = Canceled;
      }
      else {
        ret = CanceledAndUpdateIndexFailed;
        if (verbose_functor_) {
          OutputVerboseInfo("update index file failed\r\n");
        }
      }
      break;
    }

    if (UpdateIndexFile(index_file_path_)) {
      ret = Failed;
    }
    else {
      ret = FailedAndUpdateIndexFailed;
      if (verbose_functor_) {
        OutputVerboseInfo("update index file failed\r\n");
      }
    }
  } while (false);

  Destory();

  return ret;
}

void SliceManage::Stop() {
  std::lock_guard<std::mutex> lg(stop_mutex_);
  stop_ = true;
}

utf8string SliceManage::GetTargetFilePath() const {
  return target_file_path_;
}

utf8string SliceManage::GetIndexFilePath() const {
  return index_file_path_;
}

utf8string SliceManage::GetUrl() const {
  return url_;
}

void SliceManage::OutputVerboseInfo(const utf8string& info) {
  if (verbose_functor_) {
    verbose_functor_(info);
  }
}

static size_t QueryFileSizeCallback(char* buffer, size_t size, size_t nitems, void* outstream) {
  size_t avaliable = size * nitems;
  return (avaliable);
}

long SliceManage::QueryFileSize() const {
  ScopedCurl scoped_curl;
  CURL* curl = scoped_curl.GetCurl();

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl, CURLOPT_HEADER, 1);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(
      curl, CURLOPT_CONNECTTIMEOUT_MS,
      network_conn_timeout_);  // Time-out connect operations after this amount of seconds
  curl_easy_setopt(
      curl, CURLOPT_TIMEOUT_MS,
      network_read_timeout_);  // Time-out the read operation after this amount of seconds

  //if (ca_path_.length() > 0)
  //    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path_.c_str());

  // avoid libcurl failed with "Failed writing body".
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, QueryFileSizeCallback);

  CURLcode ret_code = curl_easy_perform(curl);
  if (ret_code != CURLE_OK) {
    return -1;
  }

  int http_code = 0;
  ret_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (ret_code == CURLE_OK) {
    if (http_code != 200 &&
        // A 350 response code is sent by the server in response to a file-related command that
        // requires further commands in order for the operation to be completed
        http_code != 350) {
      return -1;
    }
  }
  else {
    return -1;
  }

  int64_t file_size = 0;
  ret_code = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &file_size);
  if (ret_code != CURLE_OK) {
    return -1;
  }
  return (long)file_size;
}

bool SliceManage::LoadSlices(const utf8string& index_file_path,
                             const utf8string url,
                             ProgressFunctor functor) {
  FILE* file = OpenFile(index_file_path, u8"rb");
  if (!file)
    return false;
  long file_size = GetFileSize(file);
  fseek(file, 0, SEEK_SET);
  std::vector<char> file_content(file_size + 1, 0);
  fread(file_content.data(), 1, file_size, file);
  fclose(file);

  std::lock_guard<std::recursive_mutex> lg(slices_mutex_);

  try {
    utf8string str_sign(file_content.data(), strlen(INDEX_FILE_SIGN_STRING));
    if (str_sign != INDEX_FILE_SIGN_STRING)
      return false;

    utf8string str_json(file_content.data() + strlen(INDEX_FILE_SIGN_STRING));
    json j = json::parse(str_json);

    time_t last_update_time = j["update_time"].get<time_t>();
    if (slice_expired_seconds_ >= 0) {
      time_t now = time(nullptr);
      if (now - last_update_time > slice_expired_seconds_)
        return false;
    }

    file_size_ = j["file_size"];
    target_tmp_file_path_ = j["target_tmp_file_path"].get<utf8string>();

    if (!target_file_->Open(target_tmp_file_path_)) {
      target_tmp_file_path_.clear();
      return false;
    }

    // TODO: Support url change
    //
    if (j["url"].get<utf8string>() != url)
      return false;
    long each_slice_disk_cache_size = 0L;
    if (j["slices"].size() > 0)
      each_slice_disk_cache_size = disk_cache_total_size_ / j["slices"].size();

    for (auto& it : j["slices"]) {
      std::shared_ptr<Slice> slice = std::make_shared<Slice>(9999, shared_from_this());
      Result r = slice->Init(target_file_, it["begin"].get<long>(), it["end"].get<long>(),
                             it["capacity"].get<long>(), each_slice_disk_cache_size);
      if (r != Successed) {
        file_size_ = -1;
        slices_.clear();
        return false;
      }
      slices_.push_back(slice);
    }
  } catch (const std::exception& e) {
    if (e.what())
      std::cerr << e.what() << std::endl;
    file_size_ = -1;
    slices_.clear();
    return false;
  }
  return true;
}

bool SliceManage::UpdateIndexFile(const utf8string& index_file_path) {
  FILE* f = OpenFile(index_file_path, u8"wb");
  if (!f)
    return false;
  json j;
  j["update_time"] = time(nullptr);
  j["file_size"] = file_size_;
  j["url"] = url_;
  j["target_tmp_file_path"] = target_tmp_file_path_;

  json s;
  std::lock_guard<std::recursive_mutex> lg(slices_mutex_);
  for (auto slice : slices_) {
    s.push_back(
        {{"begin", slice->begin()}, {"end", slice->end()}, {"capacity", slice->capacity()}});
  }
  j["slices"] = s;
  utf8string str_json = j.dump();
  fwrite(INDEX_FILE_SIGN_STRING, 1, strlen(INDEX_FILE_SIGN_STRING), f);
  fwrite(str_json.c_str(), 1, str_json.size(), f);
  fclose(f);

  return true;
}

void SliceManage::Destory() {
  std::lock_guard<std::recursive_mutex> lg(slices_mutex_);
  for (auto s : slices_)
    s->UnInitCURL(multi_);

  if (multi_) {
    curl_multi_cleanup(multi_);
    multi_ = nullptr;
  }

  slices_.clear();

  {
    std::lock_guard<std::mutex> lg(stop_mutex_);
    stop_ = true;
  }
}

Result SliceManage::GenerateIndexFilePath(const utf8string& target_file_path,
                                          utf8string& index_path) const {
  utf8string target_dir = GetDirectory(target_file_path);
  if (target_dir.length() > 0) {
    if (!CreateDirectories(target_dir))
      return CreateSliceIndexDirectoryFailed;
  }
  utf8string target_filename = GetFileName(target_file_path);

  utf8string indexfilename = target_filename + ".efdindex";
  index_path = AppendFileName(target_dir, indexfilename);
  return Successed;
}

void SliceManage::ProgressNotifyThreadProc() {
  while (true) {
    {
      std::unique_lock<std::mutex> ul(stop_mutex_);
      stop_cond_var_.wait_for(ul, std::chrono::milliseconds(500), [this] { return stop_; });
      if (stop_)
        return;
    }

    long downloaded = 0;
    do {
      std::lock_guard<std::recursive_mutex> lg(slices_mutex_);
      for (const auto& s : slices_) {
        downloaded += (s->capacity() + s->diskCacheCapacity());
      }
    } while (false);

    if (progress_functor_)
      progress_functor_(file_size_, downloaded);
  }
}

void SliceManage::SpeedNotifyThreadProc(long init_total_capacity) {
  while (true) {
    {
      std::unique_lock<std::mutex> ul(stop_mutex_);
      stop_cond_var_.wait_for(ul, std::chrono::milliseconds(1000), [this] { return stop_; });
      if (stop_)
        return;
    }

    long now = 0;
    do {
      std::lock_guard<std::recursive_mutex> lg(slices_mutex_);
      for (const auto& s : slices_)
        now += (s->capacity() + s->diskCacheCapacity());
    } while (false);

    static long last = init_total_capacity;
    if (now >= last) {
      long downloaded = now - last;
      last = now;
      if (speed_functor_)
        speed_functor_(downloaded);
    }
  }
}

}  // namespace teemo
