#pragma once
#include "account/AccountManagerFanLX.h"
#include "friend/FriendDirectoryFanLX.h"
#include "group/GroupServiceFanLX.h"
#include <cstdint>
#include <map>
#include <string>

// 应用私有持有在线聚合体，外部只获得 const 视图。
struct PlatformStateFanLX {
    AccountManagerFanLX accounts;
    FriendDirectoryFanLX friends;
    GroupServiceFanLX groups;
    std::uint64_t generation = 0, parentGeneration = 0;
    std::string committedAtUtc;
    std::map<std::string, std::uint64_t> versions;
};
