#include "persistence/DataDirectoryLockFanLX.h"
#include "persistence/PersistenceErrorFanLX.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

DataDirectoryLockFanLX::DataDirectoryLockFanLX(const std::filesystem::path &directory) {
    std::filesystem::create_directories(directory);
    HANDLE h = CreateFileW((directory / L".lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        throw PersistenceErrorFanLX(error == ERROR_SHARING_VIOLATION ? PersistenceErrorCodeFanLX::Locked
                                                                     : PersistenceErrorCodeFanLX::Storage,
                                    "无法独占数据目录，Win32=" + std::to_string(error));
    }
    handleFanLX = h;
}
void DataDirectoryLockFanLX::close() noexcept {
    if (handleFanLX) {
        CloseHandle(static_cast<HANDLE>(handleFanLX));
        handleFanLX = nullptr;
    }
}
DataDirectoryLockFanLX::~DataDirectoryLockFanLX() {
    close();
}
