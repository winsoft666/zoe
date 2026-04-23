# Zoe

A high-performance C++ file download library.

[English](README.md) | [简体中文](README_ch.md)

## Features

- Multi-protocol support (HTTP/HTTPS, FTP/FTPS)
- Multi-threaded segmented downloads with resume capability
- Support for large files (up to PB level)
- Configurable download speed limits
- Progress monitoring and real-time speed tracking
- Considered the slow read and write speeds of HDD and the limited write lifespan of SSD.
- Hash verification (MD5, SHA1, SHA256)
- SSL/TLS certificate verification
- Proxy support
- Cross-platform (Windows, Linux, macOS)

## Building and Installation

Zoe's only dependency is [curl](https://github.com/curl/curl).

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

## Usage

### Basic Usage

```cpp
#include <zoe/zoe.h>

int main() {
    Zoe z;
    
    // Configure download settings
    z.setThreadNum(4);  // Use 4 threads
    z.setMaxDownloadSpeed(1024 * 1024);  // Limit speed to 1MB/s
    z.setMinDownloadSpeed(512 * 1024, 5000);  // Require at least 512KB/s for 5 seconds
    
    // Start download
    auto future = z.start(
        "https://example.com/file.zip",
        "file.zip",
        [](ZoeResult result) {
            if (result == ZoeResult::SUCCESSED) {
                std::cout << "Download completed successfully" << std::endl;
            } else {
                std::cout << "Download failed with error code: " << (int)result << std::endl;
            }
        },
        [](int64_t total, int64_t downloaded) {
            if (total > 0) {
                int progress = (int)((double)downloaded * 100.0 / total);
                std::cout << "Progress: " << progress << "%" << std::endl;
            }
        },
        [](int64_t bytes_per_second) {
            std::cout << "Current speed: " << bytes_per_second << " bytes/second" << std::endl;
        }
    );
    
    // Wait for download to complete
    future.wait();
    
    return 0;
}
```

### Command-line Tool

Zoe also provides a command-line tool for downloading files:

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

## Support

If you find this project helpful, please consider supporting it through my GitHub homepage.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.