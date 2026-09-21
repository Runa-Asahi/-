#pragma once
#include "group/GroupModeFanLX.h"
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

// GroupKeyFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct GroupKeyFanLX {
    GroupModeFanLX mode;
    std::string id;
    bool operator==(const GroupKeyFanLX &o) const noexcept {
        return mode == o.mode && id == o.id;
    }
};
