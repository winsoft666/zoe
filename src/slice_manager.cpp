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

#include "slice_manager.h"
#include <array>
#include <algorithm>
#include <sstream>
#include <iostream>
#include "json.hpp"
#include "file_util.h"
#include "string_helper.hpp"
#include "curl/curl.h"
#include "curl_utils.h"
#include "options.h"
#include "string_encode.h"
#include "verbose.h"

using json = nlohmann::json;

#define INDEX_FILE_SIGN_STRING "TEEMO:EASY-FILE-DOWNLOAD(2.0)"
#define TMP_FILE_EXTENSION ".teemo"

namespace teemo {
SliceManager::SliceManager(Options* options, const utf8string& redirect_url)
    : options_(options)
    , redirect_url_(redirect_url)
    , origin_file_size_(0L)
    , target_file_(nullptr) {
  index_file_path_ = makeIndexFilePath();
}

SliceManager::~SliceManager() {
  target_file_.reset();
}

std::shared_ptr<Slice> SliceManager::fetchUsefulSlice(
    bool remove_completed_slice,
    void* mult) {
  std::shared_ptr<Slice> ret_slice;
  for (auto& s : slices_) {
    if (s->isCompleted()) {
      if (remove_completed_slice && mult)
        s->stop(mult);
    }
    else {
      if (!ret_slice && s->status() == Slice::Status::UNFETCH) {
        ret_slice = s;
        ret_slice->setFetched();
        if (remove_completed_slice && mult)
          break;
      }
    }
  }
  return ret_slice;
}

const Options* SliceManager::options() {
  return options_;
}

utf8string SliceManager::redirectUrl() {
  return redirect_url_;
}

utf8string SliceManager::indexFilePath() const {
  return index_file_path_;
}

Result SliceManager::loadExistSlice(int64_t cur_file_size,
                                    const utf8string& cur_content_md5) {
  FILE* file = FileUtil::Open(index_file_path_, u8"rb");
  if (!file)
    return OPEN_INDEX_FILE_FAILED;

  int64_t file_size = FileUtil::GetFileSize(file);
  FileUtil::Seek(file, 0, SEEK_SET);
  std::vector<char> file_content((size_t)(file_size + 1), 0);
  fread(file_content.data(), 1, (size_t)file_size, file);
  FileUtil::Close(file);

  try {
    utf8string str_sign(file_content.data(), strlen(INDEX_FILE_SIGN_STRING));
    if (str_sign != INDEX_FILE_SIGN_STRING)
      return INVALID_INDEX_FORMAT;

    utf8string str_json(file_content.data() + strlen(INDEX_FILE_SIGN_STRING));
    json j = json::parse(str_json);

    time_t last_update_time = j["update_time"].get<time_t>();
    if (options_->tmp_file_expired_time >= 0) {
      time_t now = time(nullptr);
      if (now - last_update_time > options_->tmp_file_expired_time)
        return TMP_FILE_EXPIRED;
    }

    int64_t pre_file_size = j["file_size"];
    if (pre_file_size != cur_file_size) {
      OutputVerbose(
          options_->verbose_functor,
          "[teemo] File size has changed, tmp file expired: %lld -> %lld.\n",
          pre_file_size, cur_file_size);
      return TMP_FILE_EXPIRED;
    }
    utf8string pre_content_md5 = j["content_md5"].get<utf8string>();
    if (StringCaseConvert(pre_content_md5, EasyCharToLowerA) !=
            StringCaseConvert(cur_content_md5, EasyCharToLowerA) &&
        options_->content_md5_enabled) {
      OutputVerbose(
          options_->verbose_functor,
          "[teemo] Content md5 has changed, tmp file expired: %s -> %s.\n",
          pre_content_md5.c_str(), cur_content_md5.c_str());
      return TMP_FILE_EXPIRED;
    }
    utf8string tmp_file_path = j["target_tmp_file_path"].get<utf8string>();

    if (!FileUtil::IsRW(tmp_file_path))
      return TMP_FILE_CANNOT_RW;

    std::shared_ptr<TargetFile> target_file =
        std::make_shared<TargetFile>(tmp_file_path);

    if (!target_file->open())
      return OPEN_TMP_FILE_FAILED;

    if (target_file->fileSize() != cur_file_size)
      return TMP_FILE_SIZE_ERROR;

    if (j["url"].get<utf8string>() != options_->url)
      return URL_DIFFERENT;

    if (j["redirect_url"].get<utf8string>() != redirect_url_ &&
        options_->redirected_url_check_enabled)
      return REDIRECT_URL_DIFFERENT;

    if (options_->url.length() == 0)
      options_->url = j["url"].get<utf8string>();

    slices_.clear();
    int32_t slice_index = 0;
    for (auto& it : j["slices"]) {
      std::shared_ptr<Slice> slice = std::make_shared<Slice>(
          ++slice_index, it["begin"].get<int64_t>(), it["end"].get<int64_t>(),
          it["capacity"].get<int64_t>(), shared_from_this());
      slices_.push_back(slice);
    }

    target_file_ = target_file;
  } catch (const std::exception& e) {
    OutputVerbose(options_->verbose_functor,
                  "[teemo] Load exist slice exception: %s.\n",
                  e.what() ? e.what() : "");
    slices_.clear();
    return INVALID_INDEX_FORMAT;
  }

  content_md5_ = cur_content_md5;
  origin_file_size_ = cur_file_size;
  OutputVerbose(options_->verbose_functor, "[teemo] Load exist slice success.\n");
  dumpSlice();
  return SUCCESSED;
}

bool SliceManager::flushAllSlices() {
  bool bret = true;
  for (auto& s : slices_) {
    if (!s->flushToDisk()) {
      bret = false; // not break
    }
  }
  return bret;
}

void SliceManager::setOriginFileSize(int64_t file_size) {
  origin_file_size_ = file_size;
}

int64_t SliceManager::originFileSize() const {
  return origin_file_size_;
}

void SliceManager::setContentMd5(const utf8string& md5) {
  content_md5_ = md5;
}

utf8string SliceManager::contentMd5() const {
  return content_md5_;
}

std::shared_ptr<TargetFile> SliceManager::targetFile() {
  return target_file_;
}

Result SliceManager::makeSlices(bool accept_ranges) {
  slices_.clear();
  utf8string tmp_file_path = options_->target_file_path + TMP_FILE_EXTENSION;
  if (target_file_)
    target_file_.reset();
  target_file_ = std::make_shared<TargetFile>(tmp_file_path);

  if (!target_file_->createNew(origin_file_size_))
    return CREATE_TARGET_FILE_FAILED;

  assert(origin_file_size_ > 0L || origin_file_size_ == -1L);

  if (origin_file_size_ == -1L || !accept_ranges) {
    std::shared_ptr<Slice> slice =
        std::make_shared<Slice>(0L, 0L, -1L, 0L, shared_from_this());
    slices_.push_back(slice);
  }
  else {
    int64_t slice_size = 0L;
    int64_t cur_begin = 0L;
    int64_t cur_end = 0L;
    int32_t slice_index = 0;

    if (options_->slice_policy == FixedSize) {
      slice_size = options_->slice_policy_value;
    }
    else if (options_->slice_policy == FixedNum) {
      if (options_->slice_policy_value > 0)
        slice_size = origin_file_size_ / options_->slice_policy_value;
    }
    else if (options_->slice_policy == Auto) {
      if (origin_file_size_ <= TEEMO_DEFAULT_FIXED_SLICE_SIZE_BYTE * 1.5f) {
        slice_size = origin_file_size_;
      }
      else {
        slice_size = TEEMO_DEFAULT_FIXED_SLICE_SIZE_BYTE;
      }
    }

    if (slice_size > 0L) {
      bool is_last = false;
      do {
        cur_end = std::min(cur_begin + slice_size - 1, origin_file_size_ - 1);
        // last slice contains all of remainder space.
        if (options_->slice_policy == FixedNum &&
            slice_index == options_->slice_policy_value) {
          cur_end = origin_file_size_ - 1L;
        }

        is_last = (cur_end == (origin_file_size_ - 1L));

        // TODO: control by option
        if (is_last)
          cur_end = -1L;

        std::shared_ptr<Slice> slice = std::make_shared<Slice>(
            slice_index++, cur_begin, cur_end, 0L, shared_from_this());
        slices_.push_back(slice);

        cur_begin = cur_end + 1L;
      } while (!is_last);
    }
  }

  dumpSlice();
  return SUCCESSED;
}

int64_t SliceManager::totalDownloaded() const {
  int64_t total = 0L;
  for (auto& s : slices_) {
    total += (s->capacity() + s->diskCacheCapacity());
  }
  return total;
}

Result SliceManager::isAllSliceCompleted(bool need_check_hash) const {
  if (origin_file_size_ != -1L && totalDownloaded() != origin_file_size_) {
    OutputVerbose(options_->verbose_functor, "[teemo] Slice total size error.\n");
    return SLICE_DOWNLOAD_FAILED;
  }

  if (!need_check_hash) {
    OutputVerbose(options_->verbose_functor, "[teemo] Do not need check hash.\n");
    return SUCCESSED;
  }

  if (options_->hash_value.length() > 0) {
    if (options_->hash_verify_policy == ALWAYS ||
        (options_->hash_verify_policy == ONLY_NO_FILESIZE &&
         origin_file_size_ == -1L)) {
      if (target_file_) {
        utf8string str_hash;
        OutputVerbose(options_->verbose_functor,
                      u8"[teemo] Start calculate temp file hash.\n");
        if (target_file_->calculateFileHash(options_, str_hash) == SUCCESSED) {
          str_hash = StringCaseConvert(str_hash, EasyCharToLowerA);
          OutputVerbose(options_->verbose_functor, "[teemo] Temp file hash: %s.\n",
                        str_hash.c_str());
          if (str_hash !=
              StringCaseConvert(options_->hash_value, EasyCharToLowerA))
            return HASH_VERIFY_NOT_PASS;
        }
        else {
          OutputVerbose(options_->verbose_functor,
                        "[teemo] Calculate temp file hash failed.\n");
          return CALCULATE_HASH_FAILED;
        }
      }
    }
  }
  else if (content_md5_.length() > 0 && options_->content_md5_enabled) {
    OutputVerbose(options_->verbose_functor,
                  "[teemo] Start calculate temp file md5.\n");
    utf8string str_md5;
    if (target_file_->calculateFileMd5(options_, str_md5) == SUCCESSED) {
      str_md5 = StringCaseConvert(str_md5, EasyCharToLowerA);
      OutputVerbose(options_->verbose_functor, "[teemo] Temp file md5: %s.\n",
                    str_md5.c_str());

      if (str_md5 != StringCaseConvert(content_md5_, EasyCharToLowerA))
        return HASH_VERIFY_NOT_PASS;
    }
    else {
      OutputVerbose(options_->verbose_functor,
                    "[teemo] Calculate temp file md5 failed.\n");
      return CALCULATE_HASH_FAILED;
    }
  }

  return SUCCESSED;
}

Result SliceManager::finishDownloadProgress(bool need_check_completed) {
  // first of all, flush buffer to disk
  OutputVerbose(options_->verbose_functor,
                "[teemo] Start flushing cache to disk.\n");
  Result flush_disk_ret = SUCCESSED;
  for (auto& s : slices_) {
    if (!s->flushToDisk()) {
      flush_disk_ret = FLUSH_TMP_FILE_FAILED;
    }
  }

  // then flush index file.
  if (!flushIndexFile()) {
    OutputVerbose(options_->verbose_functor,
                  "[teemo] Flush index file failed.\n");
  }

  // check flush disk result
  if (flush_disk_ret != SUCCESSED) {
    return flush_disk_ret;
  }

  if (need_check_completed) {
    // is all slice download completed?
    Result completed_ret = isAllSliceCompleted(true);
    if (completed_ret != SUCCESSED) {
      return completed_ret;
    }
  }

  if (!target_file_->renameTo(options_, options_->target_file_path, false)) {
    unsigned int error_code = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    error_code = GetLastError();
#endif
    OutputVerbose(options_->verbose_functor,
                  "[teemo] Rename file failed, GLE: %u, %s => %s.\n", error_code,
                  target_file_->filePath().c_str(),
                  options_->target_file_path.c_str());
    return RENAME_TMP_FILE_FAILED;
  }

  if (!FileUtil::RemoveFile(index_file_path_)) {
    // not failed
    OutputVerbose(options_->verbose_functor,
                  u8"[teemo] Remove index file failed.\n");
  }

  return SUCCESSED;
}

int32_t SliceManager::usefulSliceNum() const {
  int32_t num = 0;
  for (auto& it : slices_) {
    if (!it->isCompleted() && it->status() == Slice::UNFETCH)
      num++;
  }
  return num;
}

bool SliceManager::flushIndexFile() {
  if (index_file_path_.length() == 0)
    return false;
  FILE* f = FileUtil::Open(index_file_path_, u8"wb");
  if (!f)
    return false;
  json j;
  j["update_time"] = time(nullptr);
  j["file_size"] = origin_file_size_;
  j["content_md5"] = content_md5_;
  j["url"] = options_->url;
  j["redirect_url"] = redirect_url_;
  j["target_tmp_file_path"] = target_file_->filePath();

  json s;
  for (auto& slice : slices_) {
    s.push_back({{"begin", slice->begin()},
                 {"end", slice->end()},
                 {"capacity", slice->capacity()}});
  }
  j["slices"] = s;
  utf8string str_json = j.dump();
  fwrite(INDEX_FILE_SIGN_STRING, 1, strlen(INDEX_FILE_SIGN_STRING), f);
  fwrite(str_json.c_str(), 1, str_json.size(), f);
  fflush(f);
  FileUtil::Close(f);

  return true;
}

utf8string SliceManager::makeIndexFilePath() const {
  utf8string target_dir = FileUtil::GetDirectory(options_->target_file_path);
  utf8string target_filename =
      FileUtil::GetFileName(options_->target_file_path);
  return FileUtil::AppendFileName(target_dir, target_filename + ".efdindex");
}

void SliceManager::dumpSlice() const {
  std::stringstream ss;
  int32_t s_index = 0;
  for (auto& s : slices_) {
    ss << "<" << s_index++ << "> [" << s->begin() << "~" << s->end();
    if (s->end() == -1) {
      ss << "] (*), Disk: " << s->capacity()
         << ", Buffer: " << s->diskCacheCapacity() << "\r\n";
    }
    else {
      ss << "] (" << s->end() - s->begin() << "), Disk: " << s->capacity()
         << ", Buffer: " << s->diskCacheCapacity() << "\r\n";
    }
  }

  OutputVerbose(options_->verbose_functor, ss.str().c_str());
}
}  // namespace teemo
