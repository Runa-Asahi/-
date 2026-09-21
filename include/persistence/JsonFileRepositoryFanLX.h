#pragma once
#include "persistence/CommitStageFanLX.h"
#include "persistence/DataDirectoryLockFanLX.h"
#include "persistence/RepositoryFanLX.h"
class JsonFileRepositoryFanLX final : public RepositoryFanLX {
    std::filesystem::path directoryFanLX;
    DataDirectoryLockFanLX lockFanLX;
    bool writableFanLX = false;
    bool closedFanLX = false;
    std::uint64_t generationFanLX = 0;
    std::string committedBytesFanLX;
    LoadStatusFanLX statusFanLX = LoadStatusFanLX::Empty;
    CommitHookFanLX hookFanLX;

  public:
    explicit JsonFileRepositoryFanLX(std::filesystem::path directory, CommitHookFanLX hook = {});
    LoadResultFanLX load() override;
    void initialize() override;
    LoadResultFanLX recover() override;
    void commit(const PlatformSnapshotFanLX &snapshot) override;
    bool writable() const noexcept override {
        return writableFanLX && !closedFanLX;
    }
    void close() noexcept override;
    const std::filesystem::path &directory() const noexcept {
        return directoryFanLX;
    }
};
