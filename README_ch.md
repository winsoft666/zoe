# Zoe

一个高性能的 C++ 文件下载库。

[English](README.md) | [简体中文](README_ch.md)

## 特性

- 多协议支持（HTTP/HTTPS、FTP/FTPS）
- 多线程分片下载，支持断点续传
- 支持超大文件（可达 PB 级别）
- 可配置的下载速度限制
- 进度监控和实时速度跟踪
- 哈希校验（MD5、SHA1、SHA256）
- SSL/TLS 证书验证
- 代理支持
- 跨平台（Windows、Linux、macOS）

## 构建与安装

Zoe 的唯一依赖是 [curl](https://github.com/curl/curl)。

### vcpkg
```bash
vcpkg install zoe
```

### Windows

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make
```

## 使用方法

### 基本用法

```cpp
#include <zoe/zoe.h>

int main() {
    Zoe z;
    
    // 配置下载设置
    z.setThreadNum(3);  // 设置并发线程数
    z.setNetworkConnectionTimeout(5000);  // 设置连接超时时间为5秒
    z.setRetryTimesOfFetchFileInfo(3);  // 设置获取文件信息的重试次数
    z.setFetchFileInfoHeadMethodEnabled(true);  // 启用HEAD方法获取文件信息
    z.setExpiredTimeOfTmpFile(3600);  // 设置临时文件过期时间为1小时
    z.setMaxDownloadSpeed(1024 * 1024);  // 设置最大下载速度为1MB/s
    z.setMinDownloadSpeed(1024 * 100, 10);  // 设置最小下载速度为100KB/s，持续10秒
    z.setDiskCacheSize(10 * 1024 * 1024);  // 设置磁盘缓存大小为10MB
    z.setRedirectedUrlCheckEnabled(true);  // 启用重定向URL检查
    z.setContentMd5Enabled(true);  // 启用Content-MD5验证
    z.setSlicePolicy(SlicePolicy::FixedNum, 10);  // 设置分片策略为固定分片数
    z.setHashVerifyPolicy(HashVerifyPolicy::AlwaysVerify, HashType::MD5, "md5_value");  // 设置哈希验证
    z.setProxy("http://proxy.example.com:8080");  // 设置代理
    z.setVerifyCAEnabled(true, "ca_path");  // 启用CA验证
    z.setVerifyHostEnabled(true);  // 启用主机验证
    z.setUncompletedSliceSavePolicy(UncompletedSliceSavePolicy::ALWAYS_SAVE);  // 设置未完成分片保存策略
    z.setVerboseOutput([](const utf8string& verbose) {  // 设置详细输出
      printf("%s", verbose.c_str());
    });

    // 开始下载
    auto future = z.start(
        "http://example.com/file.zip",
        "file.zip",
        [](ZoeResult result) {  // 结果回调
          printf("Result: %d\n", (int)result);
        },
        [](int64_t total, int64_t downloaded) {  // 进度回调
          if (total > 0)
            printf("%3d%%\b\b\b\b", (int)((double)downloaded * 100.f / (double)total));
        },
        [](int64_t bytes_per_second) {  // 速度回调
          printf("%.3f MB/s\b\b\b\b\b\b\b\b\b\b", (float)bytes_per_second / (1024.f * 1024.f));
        });

    // 等待下载完成
    future.wait();
    
    return 0;
}
```

### 命令行工具

Zoe 还提供了一个命令行工具用于下载文件：

```bash
zoe_tool URL TargetFilePath [ThreadNum] [DiskCacheMb] [MD5] [TmpExpiredSeconds] [MaxSpeed]
```

选项：
- URL: 下载链接
- TargetFilePath: 下载的目标文件保存路径
- ThreadNum: 线程数量，可选，默认为1
- DiskCacheMb: 磁盘缓存大小，单位Mb，默认为20Mb
- MD5: 下载文件的MD5，可选，若不为空，则在下载完成之后会进行文件MD5校验
- TmpExpiredSeconds: 秒数，可选，临时文件经过多少秒之后过期
- MaxSpeed: 最高下载速度(byte/s)

## 支持

如果您觉得这个项目有帮助，请考虑通过我的 GitHub 主页支持它。

## 许可证

本项目采用 MIT 许可证 - 详情请参阅 [LICENSE](LICENSE) 文件。