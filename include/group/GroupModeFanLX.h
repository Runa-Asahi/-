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

// GroupModeFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
enum class GroupModeFanLX {
    QQ,    // QQ 环境
    Wechat // 微信环境
};
