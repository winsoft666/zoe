[中文版](README_ch.md)
# Project Introduction
Although there are many mature and powerful download tools at present, such as `Free Download Manager`, `Aria2`, etc. However when I want to find a library that support multiple protocols (such as http, ftp), multi-threaded download, breakpoint resume download, cross-platform, I realize that this is difficult to find a satisfactory library, especially developed by C++. 

So I developed this download library named "EasyFileDownload" based on libcurl, which can support the following features:

1. Support Multi-protocol. Since it is based on libcurl, so it supports all protocols that supported by libcurl, such as http, https, ftp, etc.

2. Support multi-threaded download.

3. Support breakpoint resume.

4. Support for obtaining real-time download rate.

5. Support download speed limit.


# Compile
### 1. Install Dependent Library
#### 1.1 libcurl

Install via vcpkg:
```
vcpkg install curl[non-http]
```

#### 1.2 cpprestsdk
`EasyFileDownload` depend on pplx that give you access to the Concurrency Runtime, a concurrent programming framework for C++, pplx is a part of `cpprestsdk` library.

Install via vcpkg:
```
vcpkg install cpprestsdk
```

#### 1.3 gtest
unit test project depend on gtest.

Install via vcpkg:
```
vcpkg install gtest
```

## 2. Begin Compiling
Firstly using CMake to generate project or makefile, then comiple.

```bash
# Windows Sample
cmake.exe -G "Visual Studio 15 2017" -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build

# Linux Sample
cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON
make
```

# easy_download_tool command line tool
`easy_download_tool` is command line download tool based on `EasyFileDownload` library. Usage:

```
easy_download_tool URL TargetFilePath [ThreadNum] [MD5] [EnableSaveSliceToTmp] [SliceCacheExpiredSeconds]
```

- URL: Download URL.
- TargetFilePath: target file saved path.
- ThreadNum: thread number, optional, default is `1`.
- MD5: target file md5, optional, if this value isn't empty, tools will check file md5 after download finished.
- EnableSaveSliceToTmp: 0 or 1, optional, whether save slice file to system temp directory or not, Windows system is the path returned by `GetTempPath` API, Linux is `/var/tmp/`.
- SliceCacheExpiredSeconds: seconds, optional, slice cache file expired after these senconds.