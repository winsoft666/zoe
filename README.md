[![Build Status](https://travis-ci.com/winsoft666/teemo.svg?branch=master)](https://travis-ci.com/winsoft666/teemo) 
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/teemo)
[![badge](https://img.shields.io/badge/license-GUN-blue)](https://github.com/winsoft666/teemo/blob/master/LICENSE)

English | [ 简体中文](README_ch.md)

# 1. Introduction
There are many mature and powerful download tools at present, such as `Free Download Manager`, `Aria2`, etc. However when I want to find a library that support multiple protocols (such as http, ftp), multi-threaded download, breakpoint resume download, cross-platform, I realize that this is difficult to find a satisfactory library, especially developed by C++. 

So I developed this download library named `"teemo"` based on libcurl, which can support the following features:

✅ Support Multi-protocol. Since teemo based on libcurl, so it supports all protocols that same as libcurl.

✅ Support multi-threaded download.

✅ Support breakpoint resume.

✅ Support for obtaining real-time download rate.

✅ Support download speed limit.

✅ Support disk cache.

---

# 2. Compile and Install
## Method 1: Using with vcpkg
The `teemo` library has been included in Microsoft's [vcpkg](https://github.com/microsoft/vcpkg/tree/master/ports/teemo), you can use the following command to install `teemo`:
```bash
vcpkg install teemo:x86-windows
```

> About vcpkg: [https://github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg)

## Method 2: Compile from source code
### Step 1: Install dependencies
I prefer to use `vcpkg` to install dependencies. Of course, this is not the only way, you can install dependencies through any ways.

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

### Step 2: Compile teemo
Firstly using CMake to generate project or makefile, then comiple it:

```bash
# Windows Sample
cmake.exe -G "Visual Studio 15 2017" -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build

# Linux Sample
cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON
make
make install
```

---

# 3. Getting Started
```c++
#include <iostream>
#include "teemo.h"

int main(int argc, char** argv) {
  using namespace teemo;

  Teemo::GlobalInit();

  Teemo efd;

  efd.setThreadNum(10);                     // Optional
  efd.setTmpFileExpiredTime(3600);          // Optional
  efd.setDiskCacheSize(20 * (2 << 19));     // Optional
  efd.setMaxDownloadSpeed(50 * (2 << 19));  // Optional

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
`teemo` is command-line download tool based on `teemo` library. 

Usage:
```bash
teemo_tool URL TargetFilePath [ThreadNum] [DiskCacheMb] [MD5] [TmpExpiredSeconds] [MaxSpeed]
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

This project does not accept donations, Thank you for your kindness, If you think this project helped you, you can give the project a star.

