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

// FriendshipFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
struct FriendshipFanLX {
    std::string ownerAccountKey; // 关系发起方（主人）的账号键
    std::string peerAccountKey;  // 关系对方（朋友）的账号键
    std::string remark;          // 主人给对方起的备注名
    std::string tag;             // 主人给对方打的分组标签
};
