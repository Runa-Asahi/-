#pragma once
#include "group/GroupRoleFanLX.h"
#include <cstddef>
#include <optional>
#include <string>

// 空时间表示迁移前未知的历史时间，不能用恢复时间冒充加入时间。
struct GroupMembershipFanLX {
    GroupRoleFanLX role;
    std::size_t joinOrder;
    std::optional<std::string> joinedAtUtc = std::nullopt;
};
