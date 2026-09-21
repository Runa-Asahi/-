#pragma once
#include "group/GroupRoleFanLX.h"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// GroupMemberSnapshotFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct GroupMemberSnapshotFanLX {
    std::string id;
    GroupRoleFanLX role;
    std::size_t joinOrder;

    bool operator==(const GroupMemberSnapshotFanLX &other) const noexcept {
        return id == other.id && role == other.role && joinOrder == other.joinOrder;
    }
};
