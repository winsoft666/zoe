#include "file_util.h"
#include <filesystem>

namespace easy_file_download {

#if (defined _WIN32 || defined WIN32)
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

    long GetFileSize(FILE *f) {
        if (!f)
            return 0;
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        return fsize;
    }

    std::string GetDirectory(const std::string &path) {
        std::string::size_type pos = path.find_last_of(PATH_SEPARATOR);
        return path.substr(0, pos);
    }

    std::string GetFileName(const std::string &path) {
        std::string::size_type pos = path.find_last_of(PATH_SEPARATOR);
        if (pos == std::string::npos)
            pos = 0;
        else
            pos++;
        return path.substr(pos);
    }

    std::string AppendFileName(const std::string &dir, const std::string &filename) {
        std::string result = dir;
        if (result.length() > 0) {
            if (result[result.length() - 1] != PATH_SEPARATOR)
                result += PATH_SEPARATOR;
        }

        result += filename;
        return result;
    }

}