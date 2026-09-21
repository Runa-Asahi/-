#include "persistence/AtomicFileWriterFanLX.h"
#include "persistence/PersistenceErrorFanLX.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <atomic>
#include <fstream>
#include <windows.h>

namespace {
bool moveFileFanLX(const std::filesystem::path &from, const std::filesystem::path &to, DWORD flags) {
    // 高频替换可能遇到短暂的外部文件占用；最多重试 190 ms。
    // 真正只读目标立即报错，不擅自清除属性。
    for (unsigned attempt = 0; attempt < 20; ++attempt) {
        if (MoveFileExW(from.c_str(), to.c_str(), flags))
            return true;
        const DWORD error = GetLastError();
        const DWORD attributes = GetFileAttributesW(to.c_str());
        if ((error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED) ||
            (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY)) ||
            attempt == 19) {
            SetLastError(error);
            return false;
        }
        Sleep(10);
    }
    return false;
}
[[noreturn]] void failureFanLX(const char *step) {
    throw PersistenceErrorFanLX(PersistenceErrorCodeFanLX::Storage,
                                std::string(step) + "，Win32=" + std::to_string(GetLastError()));
}
void hookFanLX(const CommitHookFanLX &hook, CommitStageFanLX stage) {
    if (hook)
        hook(stage);
}
std::filesystem::path temporaryFanLX(const std::filesystem::path &dir, const wchar_t *prefix) {
    static std::atomic<unsigned long long> sequence{0};
    return dir / (std::wstring(prefix) + std::to_wstring(GetCurrentProcessId()) + L"." +
                  std::to_wstring(GetTickCount64()) + L"." + std::to_wstring(++sequence));
}
} // namespace
std::string AtomicFileWriterFanLX::read(const std::filesystem::path &file) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
        throw PersistenceErrorFanLX(PersistenceErrorCodeFanLX::Storage, "读取快照失败");
    std::string bytes;
    char chunk[8192];
    while (in.read(chunk, sizeof(chunk)) || in.gcount()) {
        if (bytes.size() + static_cast<std::size_t>(in.gcount()) > 64u * 1024u * 1024u)
            throw PersistenceErrorFanLX(PersistenceErrorCodeFanLX::InvalidData, "快照超过 64 MiB");
        bytes.append(chunk, static_cast<std::size_t>(in.gcount()));
    }
    if (!in.eof())
        throw PersistenceErrorFanLX(PersistenceErrorCodeFanLX::Storage, "快照读取中断");
    return bytes;
}
void AtomicFileWriterFanLX::writeNew(const std::filesystem::path &file, const std::string &bytes,
                                     const CommitHookFanLX &hook) {
    hookFanLX(hook, CommitStageFanLX::BeforeWrite);
    HANDLE handle =
        CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        failureFanLX("创建临时文件失败");
    try {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            DWORD written = 0;
            const DWORD count = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - offset, 8192));
            if (!WriteFile(handle, bytes.data() + offset, count, &written, nullptr) || written == 0)
                failureFanLX("写入失败");
            offset += written;
            hookFanLX(hook, CommitStageFanLX::DuringWrite);
        }
        hookFanLX(hook, CommitStageFanLX::BeforeFlush);
        if (!FlushFileBuffers(handle))
            failureFanLX("刷新失败");
    } catch (...) {
        CloseHandle(handle);
        throw;
    }
    if (!CloseHandle(handle))
        failureFanLX("关闭文件失败");
}
void AtomicFileWriterFanLX::publish(const std::filesystem::path &dir, const std::string &bytes, bool backup,
                                    const CommitHookFanLX &hook) {
    const auto main = dir / L"state.json", prev = dir / L"state.json.prev";
    const auto temp = temporaryFanLX(dir, L".tmp.");
    writeNew(temp, bytes, hook);
    hookFanLX(hook, CommitStageFanLX::AfterTemporary);
    const bool exists = std::filesystem::exists(main);
    if (backup && exists) {
        hookFanLX(hook, CommitStageFanLX::BeforeBackup);
        const auto backupTemp = temporaryFanLX(dir, L".backup.tmp.");
        writeNew(backupTemp, read(main));
        if (!moveFileFanLX(backupTemp, prev, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            failureFanLX("发布备份失败");
        hookFanLX(hook, CommitStageFanLX::AfterBackup);
    }
    hookFanLX(hook, CommitStageFanLX::BeforePublish);
    // 提交点：既有主文件此前从未被移走。首次创建不允许覆盖意外目标。
    if (!moveFileFanLX(temp, main, MOVEFILE_WRITE_THROUGH | (exists ? MOVEFILE_REPLACE_EXISTING : 0)))
        failureFanLX("发布主快照失败");
    hookFanLX(hook, CommitStageFanLX::AfterPublish);
}
