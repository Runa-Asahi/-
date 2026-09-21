#pragma once
#include "group/GroupKeyFanLX.h"
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

// GroupKeyHashFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct GroupKeyHashFanLX {
    std::size_t operator()(const GroupKeyFanLX &k) const noexcept {
        return std::hash<std::string>{}(k.id) ^ (static_cast<std::size_t>(k.mode) << 1);
    }
};
