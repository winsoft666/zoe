<h1 align="center">Zoe</h1>

[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/zoe)
[![badge](https://img.shields.io/badge/license-GUN-blue)](https://github.com/winsoft666/zoe/blob/master/LICENSE)

English | [ 简体中文](README_ch.md)

A C++ file download library.

## Features

- Multi-protocol, such as HTTP(s), FTP(s)...

- Multi-threaded Segmented downloads and breakpoint transmission.
  
- Limit download speed.

- Disk cache.

- Support large file (PB level).

- Compatible with server leeching detection(or limit).

## Compile and Install

Zoe only depends on [curl](https://github.com/curl/curl). After installing curl, use CMake to compile and install Zoe.

In addition, the `Zoe` library has been included in Microsoft's [vcpkg](https://github.com/microsoft/vcpkg/tree/master/ports/zoe), which can be quickly installed directly using the following command:

```bash
vcpkg install zoe
```

## Getting Started

The following example uses Zoe's default configuration parameters to demonstrate how to quickly use Zoe to download file:

```cpp
#include <iostream>
#include "zoe.h"

int main(int argc, char** argv) {
  using namespace zoe;

  Zoe::GlobalInit();

  Zoe z;
  
  std::shared_future<Result> r = z.start(u8"http://xxx.xxx.com/test.exe", u8"D:\\test.exe");

  Result result = r.get();

  Zoe::GlobalUnInit();

  return 0;
}
```

The following example is a bit more complex and sets some configuration parameters and callback functions:

```cpp
#include <iostream>
#include "zoe.h"

int main(int argc, char** argv) {
  using namespace zoe;

  Zoe::GlobalInit();

  Zoe z;

  z.setThreadNum(10);                     // Optional
  z.setTmpFileExpiredTime(3600);          // Optional
  z.setDiskCacheSize(20 * (2 << 19));     // Optional
  z.setMaxDownloadSpeed(50 * (2 << 19));  // Optional
  z.setHashVerifyPolicy(ALWAYS, MD5, "6fe294c3ef4765468af4950d44c65525"); // Optional, support MD5, CRC32, SHA256
  // There are more options available, please check zoe.h
  z.setVerboseOutput([](const utf8string& verbose) { // Optional
    printf("%s\n", verbose.c_str());
  });
  z.setHttpHeaders({  // Optional
    {u8"Origin", u8"http://xxx.xxx.com"},
    {u8"User-Agent", u8"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)"}
   });
  
  std::shared_future<Result> r = z.start(
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

  r.wait();

  Zoe::GlobalUnInit();

  return 0;
}
```

## Command-line tool

`zoe_tool` is command-line tool based on `zoe` library. 

Usage:

```bash
zoe_tool URL TargetFilePath [ThreadNum] [DiskCacheMb] [MD5] [TmpExpiredSeconds] [MaxSpeed]
```

- URL: Download URL.
- TargetFilePath: target file saved path.
- ThreadNum: thread number, optional, default is `1`.
- DiskCacheMb: Disk cache size(Mb), default is `20Mb`.
- MD5: target file md5, optional, if this value isn't empty, tools will check file md5 after download finished.
- TmpExpiredSeconds: seconds, optional, the temporary file will expired after these senconds.
- MaxSpeed: max download speed(byte/s).
