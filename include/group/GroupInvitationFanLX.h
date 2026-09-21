#pragma once
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

// GroupInvitationFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct GroupInvitationFanLX {
    std::string inviter;
    std::string invitee;
    bool pending = true;
    std::string status = "Pending";
    std::optional<std::string> createdAtUtc;
    std::optional<std::string> expiresAtUtc;
};
