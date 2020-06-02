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
#include "curl_utils.h"
#include "options.h"

using json = nlohmann::json;

#define INDEX_FILE_SIGN_STRING "TEEMO:EASY-FILE-DOWNLOAD(2.0)"
#define TMP_FILE_EXTENSION ".teemo"

namespace teemo {
SliceManager::SliceManager(Options* options) : options_(options), origin_file_size_(0L) {
  index_file_path_ = makeIndexFilePath();
  target_file_ = std::make_shared<TargetFile>();
}

SliceManager::~SliceManager() {
  target_file_.reset();
}

std::shared_ptr<Slice> SliceManager::fetchUsefulSlice() {
  for (auto& s : slices_) {
    if (!s->isCompleted() && s->status() == Slice::Status::UNFETCH) {
      s->setFetched();
      return s;
    }
  }
  return nullptr;
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

    if (j["url"].get<utf8string>() != options_->url && !options_->can_update_url)
      return URL_DIFFERENT;

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
    return SUCCESSED;
  }

  if (!target_file_->IsOpened()) {
    tmp_file_path_ = options_->target_file_path + TMP_FILE_EXTENSION;
    if (!target_file_->Create(tmp_file_path_, origin_file_size_))
      return CREATE_TARGET_FILE_FAILED;
  }

  int64_t cur_begin = 0L;
  int64_t cur_end = 0L;
  int32_t slice_index = 0;
  do {
    cur_end = std::min(cur_begin + options_->fixed_slice_size, origin_file_size_ - 1);
    std::shared_ptr<Slice> slice =
        std::make_shared<Slice>(slice_index++, cur_begin, cur_end, 0L, shared_from_this());
    slices_.push_back(slice);
    cur_begin = cur_end;

    if (cur_end >= origin_file_size_ - 1)
      break;
  } while (true);

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
  for (auto& s : slices_) {
    if (!s->flushToDisk())
      ret = FLUSH_TMP_FILE_FAILED;
  }
  target_file_->Close();

  if (ret != SUCCESSED)
    return ret;

  if (isAllSliceCompleted()) {
    FileUtil::RemoveFile(index_file_path_);
    if (!FileUtil::RenameFile(tmp_file_path_, options_->target_file_path, true))
      return RENAME_TMP_FILE_FAILED;
    return SUCCESSED;
  }
  else {
    if (!flushIndexFile())
      return UPDATE_INDEX_FILE_FAILED;
  }
  return SLICE_DOWNLOAD_FAILED;
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
}  // namespace teemo
