[![Build Status](https://travis-ci.com/winsoft666/zoe.svg?branch=master)](https://travis-ci.com/winsoft666/zoe) 
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/zoe)
[![badge](https://img.shields.io/badge/license-GUN-blue)](https://github.com/winsoft666/zoe/blob/master/LICENSE)

English | [ 简体中文](README_ch.md)

# 1. Introduction

Although there are many mature and powerful download tools, such as `Free Download Manager`, `Aria2`, etc., but when I want to find a file download library that supports multiple protocols (such as http, ftp), multi-threaded, resumable, cross-platform, open source, I realized that it's a difficult work, especially developed in C++. So I developed this download library named "zoe" based on libcurl, which can support the following features:

✅ Support Multi-protocol. Since zoe based on libcurl, so it supports all protocols that same as libcurl.

✅ Support segmented downloads.

✅ Support breakpoint resumable.

✅ Support downloading pause/resume.

✅ Support for obtaining real-time download rate.

✅ Support download speed limit.

✅ Support disk cache.

✅ Support hash checksum verify.

✅ Support large file download.

✅ Support compatible server leeching detection(or limit).

---

# 2. Compile and Install
## Method 1: Using with vcpkg
The `zoe` library has been included in Microsoft's [vcpkg](https://github.com/microsoft/vcpkg/tree/master/ports/zoe), you can use the following command to install `zoe`:
- 1) Clone and setup vcpkg (See more detail on [https://github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg))
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
PS> bootstrap-vcpkg.bootstrap
Linux:~/$ ./bootstrap-vcpkg.sh
```

- 2) Install zoe
```bash
PS> .\vcpkg install zoe [--triplet x64-windows-static/x64-windows/x64-windows-static-md and etc...]
Linux:~/$ ./vcpkg install zoe
```


## Method 2: Compile from source code
### Step 1: Install dependencies
I prefer to use `vcpkg` to install dependencies. Of course, this is not the only way, you can install dependencies through any ways.

Recommend: add the directory where vcpkg.exe resides to the PATH environment variable.

- libcurl

```bash
# if you want support non-http protocol, such as ftp, the [non-http] option must be specified.
vcpkg install curl[non-http]:x86-windows
```

- gtest

unit test project depend on gtest.

```bash
vcpkg install gtest:x86-windows
```

### Step 2: Compile zoe
Firstly using CMake to generate project or makefile, then comiple it:

**Windows Sample**
```bash
cmake.exe -G "Visual Studio 15 2017" -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build

```

**Linux Sample**
```bash
cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON

# If using vcpkg to install dependencies, you have to special CMAKE_TOOLCHAIN_FILE
cmake -DCMAKE_TOOLCHAIN_FILE=/xxx/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON

make
```

---

# 3. Getting Started
```c++
#include <iostream>
#include "zoe.h"

int main(int argc, char** argv) {
  using namespace zoe;

  Teemo::GlobalInit();

  Teemo efd;

  efd.setThreadNum(10);                     // Optional
  efd.setTmpFileExpiredTime(3600);          // Optional
  efd.setDiskCacheSize(20 * (2 << 19));     // Optional
  efd.setMaxDownloadSpeed(50 * (2 << 19));  // Optional
  efd.setHashVerifyPolicy(ALWAYS, MD5, "6fe294c3ef4765468af4950d44c65525"); // Optional, support MD5, CRC32, SHA256
  efd.setVerboseOutput([](const utf8string& verbose) { // Optional
    printf("%s\n", verbose.c_str());
  });
  efd.setHttpHeaders({  // Optional
    {u8"Origin", u8"http://xxx.xxx.com"},
    {u8"User-Agent", u8"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)"}
   });
  
  std::shared_future<Result> async_task = efd.start(
      u8"http://xxx.xxx.com/test.exe", u8"D:\\test.exe",
      [](Result result) {  // Optional
        // result callback
      },
      [](int64_t total, int64_t downloaded) {  // Optional
        // progress callback
      },
      [](int64_t byte_per_secs) {  // Optional
        // real-time speed callback
      });

  async_task.wait();

  Teemo::GlobalUnInit();

  return 0;
}
```

---

# 4. Command-line tool
`zoe` is command-line download tool based on `zoe` library. 

Usage:
```bash
libGet_tool URL TargetFilePath [ThreadNum] [DiskCacheMb] [MD5] [TmpExpiredSeconds] [MaxSpeed]
```

- URL: Download URL.
- TargetFilePath: target file saved path.
- ThreadNum: thread number, optional, default is `1`.
- DiskCacheMb: Disk cache size(Mb), default is `20Mb`.
- MD5: target file md5, optional, if this value isn't empty, tools will check file md5 after download finished.
- TmpExpiredSeconds: seconds, optional, the temporary file will expired after these senconds.
- MaxSpeed: max download speed(byte/s).



---



# 5. Donate

This project does not accept donations, Thank you for your kindness!

Open source is not a easy work, if you think this project helped you, you can give the project a star⭐.

