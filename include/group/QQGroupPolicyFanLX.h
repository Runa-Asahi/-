#pragma once
#include "group/GroupPolicyFanLX.h"
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

// QQGroupPolicyFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
class QQGroupPolicyFanLX final : public GroupPolicyFanLX {
  public:
    bool canCreateDiscussion() const override {
        return true;
    } // QQ 支持群讨论
    std::string name() const override {
        return "QQ";
    } // 平台名：QQ
    GroupModeFanLX mode() const override {
        return GroupModeFanLX::QQ;
    }
    bool canAssignAdmin() const override {
        return true;
    }
};
