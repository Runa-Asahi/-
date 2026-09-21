#pragma once
#include "persistence/CommitStageFanLX.h"
#include <filesystem>
#include <string>
class AtomicFileWriterFanLX {
  public:
    static std::string read(const std::filesystem::path &file);
    static void writeNew(const std::filesystem::path &file, const std::string &bytes,
                         const CommitHookFanLX &hook = {});
    // 恢复模式不覆盖有效备份；正常模式先复制备份，主文件始终留在原位。
    static void publish(const std::filesystem::path &directory, const std::string &bytes, bool backup,
                        const CommitHookFanLX &hook = {});
};
