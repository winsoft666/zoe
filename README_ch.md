# 项目介绍
目前虽然有很多成熟且功能强大的下载工具，如Free Download Manager, Aria2等等，但当我想找一个支持多种协议(如http， ftp)、多线程下载、断点续传、跨平台的开源库时，发现很难找到满意的，特别是使用C++开发的。于是我基于libcurl开发了这个名为"EasyFileDownload"下载库，它可以支持如下特性：
1. 多协议支持，由于是基于libcurl的，所以支持libcurl所支持的所有协议，如http, https, ftp等。
2. 支持多线程下载
3. 支持断点续传
4. 支持获取实时下载速率
5. 支持下载限速

# 编译
### 1. 安装依赖项
#### 1.1 libcurl
vcpkg安装方式：
```
vcpkg install curl[non-http]
```

#### 1.2 cpprestsdk
使用了cpprestsdk库中的pplx并行开发库。

vcpkg安装方式：
```
vcpkg install cpprestsdk
```

#### 1.3 gtest
单元测试项目使用了gtest。

vcpkg安装方式：
```
vcpkg install gtest
```

## 2. 开始编译
使用CMake生成相应的工程，然后编译即可。
```bash
# Windows示例
cmake.exe -G "Visual Studio 15 2017" -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON -S %~dp0 -B %~dp0build

# Linux
cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=ON
make
```

# easy_download_tool命令行工具
`easy_download_tool`是一个基于`EasyFileDownload`库开发的命令行下载工具，用法如下：
```
easy_download_tool URL TargetFilePath [ThreadNum] [MD5] [EnableSaveSliceToTmp]
```

- URL: 下载链接
- TargetFilePath: 下载的目标文件保存路径
- ThreadNum: 线程数量，可选，默认为1
- MD5: 下载文件的MD5，可选，若不为空，则在下载完成之后会进行文件MD5校验
- EnableSaveSliceToTmp: 0或1，可选，是否保存分片文件到系统临时目录，Windows平台为`GetTempPath`API返回的路径，Linux平台为`/var/tmp/`

