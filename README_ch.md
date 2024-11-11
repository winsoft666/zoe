<h1 align="center">Zoe</h1>

[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/zoe)
[![badge](https://img.shields.io/badge/license-GUN-blue)](https://github.com/winsoft666/zoe/blob/master/LICENSE)

简体中文 | [ English](README.md)

## 特性

- 多协议支持，如 HTTP(s), FTP(s)...
  
- 分片下载和断点续传
  
- 下载限速

- 磁盘缓存

- 支持 PB 级别大文件下载

- 兼容服务器对加速下载行为的限制

## 编译与安装

Zoe 仅依赖 [curl](https://github.com/curl/curl)，在安装 curl 之后，使用 CMake 编译安装 Zoe。

另外，`Zoe` 库已经收录到微软的 [vcpkg](https://github.com/microsoft/vcpkg/tree/master/ports/zoe)，可以直接使用下面命令快速安装：

```bash
vcpkg install zoe
```

## 快速开始

```cpp
#include <iostream>
#include "zoe.h"

int main(int argc, char** argv) {
  using namespace zoe;

  Zoe::GlobalInit();

  Zoe z;

  z.setThreadNum(10);                     // 可选的
  z.setTmpFileExpiredTime(3600);          // 可选的
  z.setDiskCacheSize(20 * (2 << 19));     // 可选的
  z.setMaxDownloadSpeed(50 * (2 << 19));  // 可选的
  z.setHashVerifyPolicy(ALWAYS, MD5, "6fe294c3ef4765468af4950d44c65525"); // 可选的, 支持 MD5, CRC32, SHA256
  // 还支持其他更多的选择，请查看zoe.h
  z.setVerboseOutput([](const utf8string& verbose) { // 可选的
    printf("%s\n", verbose.c_str());
  });
  z.setHttpHeaders({  // 可选的
    {u8"Origin", u8"http://xxx.xxx.com"},
    {u8"User-Agent", u8"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)"}
   });
  
  std::shared_future<Result> res = z.start(
      u8"http://xxx.xxx.com/test.exe", u8"D:\\test.exe",
      [](Result result) {  // 可选的
        // 结果回调
      },
      [](int64_t total, int64_t downloaded) {  // 可选的
        // 进度回调
      },
      [](int64_t byte_per_secs) {  // 可选的
        // 实时速率回调
      });

  res.wait();

  Zoe::GlobalUnInit();

  return 0;
}
```

## 命令行工具
`zoe_tool`是一个基于 `zoe` 库开发的命令行下载工具，用法如下：

```bash
zoe_tool URL TargetFilePath [ThreadNum] [DiskCacheMb] [MD5] [TmpExpiredSeconds] [MaxSpeed]
```

参数解释：
- URL: 下载链接
- TargetFilePath: 下载的目标文件保存路径
- ThreadNum: 线程数量，可选，默认为1
- DiskCacheMb: 磁盘缓存大小，单位Mb，默认为20Mb
- MD5: 下载文件的MD5，可选，若不为空，则在下载完成之后会进行文件MD5校验
- TmpExpiredSeconds: 秒数，可选，临时文件经过多少秒之后过期
- MaxSpeed: 最高下载速度(byte/s)
