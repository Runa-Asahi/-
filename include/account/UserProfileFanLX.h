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

// UserProfileFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct UserProfileFanLX {
    std::string userId;    // 全局唯一用户 ID（如 U001）
    std::string nickname;  // 昵称
    std::string birthDate; // 出生日期
    std::string applyDate; // 注册 / 申请日期
    std::string location;  // 所在地
};
