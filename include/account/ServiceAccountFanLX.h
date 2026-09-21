#pragma once
#include "account/ServiceTypeFanLX.h"
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

// ServiceAccountFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct ServiceAccountFanLX {
    std::string userId;       // 归属用户的 ID（外键，关联 UserProfileFanLX）
    ServiceTypeFanLX service; // 所属平台（QQ / Wechat / Weibo）
    std::string accountId;    // 平台侧账号 ID（前端登录身份，平台内唯一）
    bool sharedId = false;    // 是否与其它平台共用账号 ID（用于统一登录识别）
};
