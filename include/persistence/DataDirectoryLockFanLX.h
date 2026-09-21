#pragma once
#include <filesystem>
// 独占句柄才是存活依据；锁文件残留不等于仍有进程占用。
class DataDirectoryLockFanLX {
    void *handleFanLX = nullptr;

  public:
    explicit DataDirectoryLockFanLX(const std::filesystem::path &directory);
    ~DataDirectoryLockFanLX();
    DataDirectoryLockFanLX(const DataDirectoryLockFanLX &) = delete;
    DataDirectoryLockFanLX &operator=(const DataDirectoryLockFanLX &) = delete;
    void close() noexcept;
};
