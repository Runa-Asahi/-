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

// GroupPolicyFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
class GroupPolicyFanLX {
  public:
    virtual ~GroupPolicyFanLX() = default; // 虚析构：保证通过基类指针删除派生类安全

    // 该平台是否允许创建群讨论。
    virtual bool canCreateDiscussion() const = 0;
    // 该策略所属平台名称。
    virtual std::string name() const = 0;
    // 当前治理模式与创建时的群键无关，切换不会重建群身份。
    virtual GroupModeFanLX mode() const = 0;
    // 同时决定能否任命管理员，以及已保存的管理员角色是否生效。
    virtual bool canAssignAdmin() const = 0;
};
#include "group/QQGroupPolicyFanLX.h"
#include "group/WechatGroupPolicyFanLX.h"
