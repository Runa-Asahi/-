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

// SessionExpiredFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct SessionExpiredFanLX : std::runtime_error {
    using std::runtime_error::runtime_error;
};
