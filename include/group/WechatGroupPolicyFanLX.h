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

// WechatGroupPolicyFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
class WechatGroupPolicyFanLX final : public GroupPolicyFanLX {
  public:
    bool canCreateDiscussion() const override {
        return false;
    } // 微信不支持群讨论
    std::string name() const override {
        return "Wechat";
    } // 平台名：Wechat
    GroupModeFanLX mode() const override {
        return GroupModeFanLX::Wechat;
    }
    bool canAssignAdmin() const override {
        return false;
    }
};
