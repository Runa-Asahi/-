#pragma once
#include "persistence/CommitStageFanLX.h"
#include "persistence/PlatformStateFanLX.h"
#include "persistence/RepositoryFanLX.h"
#include "infra/UtcClockFanLX.h"
#include <mutex>

class TransactionCoordinatorFanLX {
    RepositoryFanLX &repositoryFanLX;
    PlatformStateFanLX &stateFanLX;
    std::mutex mutexFanLX;
    std::function<std::string()> clockFanLX;
    CommitHookFanLX hookFanLX;

  public:
    TransactionCoordinatorFanLX(RepositoryFanLX &repository, PlatformStateFanLX &state,
                                std::function<std::string()> clock = utcNowFanLX, CommitHookFanLX hook = {});
    // 回调只操作候选；不得让候选引用逃逸至 UI。
    bool execute(const std::function<void(PlatformStateFanLX &)> &mutation, bool force = false);
};
