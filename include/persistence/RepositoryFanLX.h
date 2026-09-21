#pragma once
#include "persistence/LoadResultFanLX.h"
// 仓储多态接口：业务协调器不直接访问 Windows 文件 API。
class RepositoryFanLX {
  public:
    virtual ~RepositoryFanLX() = default;
    virtual LoadResultFanLX load() = 0;
    virtual void initialize() = 0;
    virtual LoadResultFanLX recover() = 0;
    virtual void commit(const PlatformSnapshotFanLX &snapshot) = 0;
    virtual bool writable() const noexcept = 0;
    virtual void close() noexcept = 0;
};
