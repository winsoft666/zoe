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
#include "curl/curl.h"
#include "curl_utils.h"
#include "options.h"
#include "md5.h"
#include "crc32.h"
#include "sha1.h"
#include "sha256.h"

using json = nlohmann::json;

#define INDEX_FILE_SIGN_STRING "TEEMO:EASY-FILE-DOWNLOAD(2.0)"
#define TMP_FILE_EXTENSION ".teemo"

namespace teemo {

namespace {
inline char EasyCharToLowerA(char in) {
  if (in <= 'Z' && in >= 'A')
    return in - ('Z' - 'z');
  return in;
}

template <typename T, typename Func>
typename std::enable_if<std::is_same<char, T>::value || std::is_same<wchar_t, T>::value,
                        std::basic_string<T, std::char_traits<T>, std::allocator<T>>>::type
StringCaseConvert(const std::basic_string<T, std::char_traits<T>, std::allocator<T>>& str,
                  Func func) {
  std::basic_string<T, std::char_traits<T>, std::allocator<T>> ret = str;
  std::transform(ret.begin(), ret.end(), ret.begin(), func);
  return ret;
}
}  // namespace
SliceManager::SliceManager(Options* options) : options_(options), origin_file_size_(0L) {
  index_file_path_ = makeIndexFilePath();
  target_file_ = std::make_shared<TargetFile>();
}

SliceManager::~SliceManager() {
  target_file_.reset();
}

std::shared_ptr<Slice> SliceManager::fetchUsefulSlice(bool remove_completed_slice, void* mult) {
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

utf8string SliceManager::indexFilePath() const {
  return index_file_path_;
}

Result SliceManager::loadExistSlice() {
  FILE* file = FileUtil::OpenFile(index_file_path_, u8"rb");
  if (!file)
    return OPEN_INDEX_FILE_FAILED;
  int64_t file_size = FileUtil::GetFileSize(file);
  fseek(file, 0, SEEK_SET);
  std::vector<char> file_content((size_t)(file_size + 1), 0);
  fread(file_content.data(), 1, (size_t)file_size, file);
  fclose(file);

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

    origin_file_size_ = j["file_size"];
    tmp_file_path_ = j["target_tmp_file_path"].get<utf8string>();

    if (!FileUtil::FileIsRW(tmp_file_path_))
      return TMP_FILE_CANNOT_RW;

    if (FileUtil::GetFileSize(tmp_file_path_) != origin_file_size_)
      return TMP_FILE_SIZE_ERROR;

    if (!target_file_->Open(tmp_file_path_))
      return OPEN_TMP_FILE_FAILED;

    if (j["url"].get<utf8string>() != options_->url && !options_->skipping_url_check)
      return URL_DIFFERENT;

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
  } catch (const std::exception& e) {
    if (e.what())
      std::cerr << e.what() << std::endl;
    origin_file_size_ = 0;
    slices_.clear();
    return INVALID_INDEX_FORMAT;
  }
  return SUCCESSED;
}

void SliceManager::setOriginFileSize(int64_t file_size) {
  origin_file_size_ = file_size;

  flushIndexFile();
}

int64_t SliceManager::originFileSize() const {
  return origin_file_size_;
}

std::shared_ptr<TargetFile> SliceManager::targetFile() {
  return target_file_;
}

Result SliceManager::tryMakeSlices() {
  if (slices_.size() > 0) {
    dumpSlice();
    return SUCCESSED;
  }

  if (!target_file_->IsOpened()) {
    tmp_file_path_ = options_->target_file_path + TMP_FILE_EXTENSION;
    if (!target_file_->Create(tmp_file_path_, origin_file_size_))
      return CREATE_TARGET_FILE_FAILED;
  }

  assert(origin_file_size_ > 0 || origin_file_size_ == -1);

  if (origin_file_size_ == -1) {
    std::shared_ptr<Slice> slice = std::make_shared<Slice>(0, 0, -1, 0L, shared_from_this());
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

    do {
      cur_end = std::min(cur_begin + slice_size, origin_file_size_ - 1);
      std::shared_ptr<Slice> slice =
          std::make_shared<Slice>(slice_index++, cur_begin, cur_end, 0L, shared_from_this());
      slices_.push_back(slice);

      // last slice contains all of remainder space.
      //
      if (options_->slice_policy == FixedNum && slice_index == options_->slice_policy_value) {
        cur_end = origin_file_size_ - 1;
      }

      cur_begin = cur_end;
      if (cur_end >= origin_file_size_ - 1)
        break;
    } while (true);
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

bool SliceManager::isAllSliceCompleted() const {
  for (auto& s : slices_) {
    if (!s->isCompleted())
      return false;
  }
  return true;
}

Result SliceManager::finishDownload() {
  Result ret = SUCCESSED;

  do {
    Result flush_ret = SUCCESSED;
    for (auto& s : slices_) {
      if (!s->flushToDisk())
        flush_ret = FLUSH_TMP_FILE_FAILED;
    }
    target_file_->Close();

    if (flush_ret != SUCCESSED) {
      ret = flush_ret;
      break;
    }

    // Can not fetch file size, we don't know if the file is actually download completed.
    if (origin_file_size_ == -1) {
      if (options_->hash_value.length() > 0) {
        utf8string str_hash;
        ret = calculateTmpFileHash(str_hash);
        if (ret != SUCCESSED) {
          break;
        }

        str_hash = StringCaseConvert(str_hash, EasyCharToLowerA);
        if (str_hash != StringCaseConvert(options_->hash_value, EasyCharToLowerA)) {
          ret = HASH_VERIFY_NOT_PASS;
          break;
        }
      }

      ret = SUCCESSED;
      break;
    }

    if (isAllSliceCompleted()) {
      if (options_->hash_value.length() > 0 && options_->hash_verify_policy == ALWAYS) {
        utf8string str_hash;
        ret = calculateTmpFileHash(str_hash);
        if (ret != SUCCESSED) {
          break;
        }

        str_hash = StringCaseConvert(str_hash, EasyCharToLowerA);
        if (str_hash != StringCaseConvert(options_->hash_value, EasyCharToLowerA)) {
          ret = HASH_VERIFY_NOT_PASS;
          break;
        }
      }

      if (!FileUtil::RenameFile(tmp_file_path_, options_->target_file_path, true)) {
        ret = RENAME_TMP_FILE_FAILED;
        break;
      }

      ret = SUCCESSED;
      break;
    }

    ret = SLICE_DOWNLOAD_FAILED;
  } while (false);

  if (ret == SUCCESSED) {
    if (!FileUtil::RemoveFile(index_file_path_)) {
      if (options_->verbose_functor)
        options_->verbose_functor("\r\nremove index file failed\r\n");
    }
  }
  else {
    if (!flushIndexFile())
      ret = UPDATE_INDEX_FILE_FAILED;
  }

  return ret;
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
  FILE* f = FileUtil::OpenFile(index_file_path_, u8"wb");
  if (!f)
    return false;
  json j;
  j["update_time"] = time(nullptr);
  j["file_size"] = origin_file_size_;
  j["url"] = options_->url;
  j["target_tmp_file_path"] = tmp_file_path_;

  json s;
  for (auto& slice : slices_) {
    s.push_back(
        {{"begin", slice->begin()}, {"end", slice->end()}, {"capacity", slice->capacity()}});
  }
  j["slices"] = s;
  utf8string str_json = j.dump();
  fwrite(INDEX_FILE_SIGN_STRING, 1, strlen(INDEX_FILE_SIGN_STRING), f);
  fwrite(str_json.c_str(), 1, str_json.size(), f);
  fflush(f);
  fclose(f);

  return true;
}

utf8string SliceManager::makeIndexFilePath() const {
  utf8string target_dir = FileUtil::GetDirectory(options_->target_file_path);
  utf8string target_filename = FileUtil::GetFileName(options_->target_file_path);
  return FileUtil::AppendFileName(target_dir, target_filename + ".efdindex");
}

Result SliceManager::calculateTmpFileHash(utf8string &str_hash) {
  Result ret = CALCULATE_HASH_FAILED;
  if (options_->hash_type == MD5) {
    ret = CalculateFileMd5(tmp_file_path_, options_, str_hash);
  }
  else if (options_->hash_type == CRC32) {
    ret = CalculateFileCRC32(tmp_file_path_, options_, str_hash);
  }
  else if (options_->hash_type == SHA1) {
    ret = CalculateFileSHA1(tmp_file_path_, options_, str_hash);
  }
  else if (options_->hash_type == SHA256) {
    ret = CalculateFileSHA256(tmp_file_path_, options_, str_hash);
  }
  return ret;
}

void SliceManager::dumpSlice() {
  if (options_->verbose_functor) {
    std::stringstream ss;
    int32_t s_index = 0;
    for (auto& s : slices_) {
      ss << "[" << s_index++ << "] " << s->begin() << "~" << s->end() << ", Disk: " << s->capacity()
         << ", Buffer: " << s->diskCacheCapacity() << "\r\n";
    }

    options_->verbose_functor(ss.str());
  }
}

}  // namespace teemo
