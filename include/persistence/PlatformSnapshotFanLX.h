#pragma once
#include "account/AccountRefFanLX.h"
#include "account/BindingSnapshotFanLX.h"
#include "account/ServiceAccountFanLX.h"
#include "account/UserProfileFanLX.h"
#include "friend/FriendSnapshotFanLX.h"
#include "group/GroupRecordFanLX.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// 纯 STL 数据传输对象，不包含运行时句柄、会话或缓存。
struct PlatformSnapshotFanLX {
    std::uint64_t generation = 0;
    std::uint64_t parentGeneration = 0;
    std::string committedAtUtc;
    std::vector<UserProfileFanLX> users;
    std::vector<ServiceAccountFanLX> accounts;
    std::vector<AccountRefFanLX> subscriptions;
    std::vector<BindingSnapshotFanLX> bindings;
    std::vector<FriendSnapshotFanLX> friendships;
    std::vector<GroupRecordFanLX> groups;
    std::vector<GroupRecordFanLX> archivedGroups;
    std::map<std::string, std::uint64_t> versions;
};
