#pragma once

#include <stdio.h>
#include <string>

namespace easy_file_download {
    long GetFileSize(FILE *f);
    std::string GetDirectory(const std::string &path);
    std::string GetFileName(const std::string &path);
    std::string AppendFileName(const std::string &dir, const std::string &filename);
}