#pragma once
#include "group/GroupKeyFanLX.h"
#include "group/GroupMemberSnapshotFanLX.h"
#include "group/GroupPolicyFanLX.h"
#include "group/GroupStateFanLX.h"
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

// GroupFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
class GroupFanLX {
    GroupKeyFanLX keyFanLX;
    std::unique_ptr<GroupPolicyFanLX> policyFanLX;
    friend class SnapshotCodecFanLX;
    friend class PreparedCommitFanLX;
    GroupStateFanLX stateFanLX;
    std::string nameFanLX;

  public:
    GroupFanLX(GroupKeyFanLX key, std::unique_ptr<GroupPolicyFanLX> policy)
        : keyFanLX(std::move(key)), policyFanLX(std::move(policy)) {}
    const GroupKeyFanLX &key() const noexcept {
        return keyFanLX;
    }
    const GroupPolicyFanLX &policy() const noexcept {
        return *policyFanLX;
    }
    bool addMember(const std::string &id, GroupRoleFanLX role = GroupRoleFanLX::Member) {
        if (stateFanLX.members.count(id))
            return false;
        if (stateFanLX.nextJoinOrder == std::numeric_limits<std::size_t>::max())
            throw std::overflow_error("加入序号耗尽");
        stateFanLX.members.emplace(id, GroupMembershipFanLX{role, stateFanLX.nextJoinOrder});
        // 实际插入成功后递增，重复请求或分配失败均不消耗序号。
        ++stateFanLX.nextJoinOrder;
        return true;
    }
    void setRole(const std::string &id, GroupRoleFanLX role) {
        stateFanLX.members.at(id).role = role;
    }
    bool hasMember(const std::string &id) const {
        return stateFanLX.members.count(id) != 0;
    }
    GroupRoleFanLX roleOf(const std::string &id) const {
        return stateFanLX.members.at(id).role;
    }
    bool removeMember(const std::string &actor, const std::string &target) {
        if (!hasMember(actor) || !hasMember(target) || actor == target ||
            roleOf(target) == GroupRoleFanLX::Owner)
            return false;
        const auto actorRole = roleOf(actor);
        const bool actorIsEffectiveAdmin =
            actorRole == GroupRoleFanLX::Admin && policyFanLX->canAssignAdmin();
        if (actorRole != GroupRoleFanLX::Owner) {
            if (!actorIsEffectiveAdmin || roleOf(target) == GroupRoleFanLX::Admin)
                return false;
        }
        stateFanLX.members.erase(target);
        return true;
    }
    GroupModeFanLX currentMode() const noexcept {
        return policyFanLX->mode();
    }

    void switchPolicy(std::unique_ptr<GroupPolicyFanLX> newPolicy) {
        if (!newPolicy)
            throw std::invalid_argument("目标策略不可为空");
        // 调用方已完成构造；仅替换行为，所有成员及 Service 中的关联数据保持原位。
        policyFanLX = std::move(newPolicy);
    }

    std::vector<GroupMemberSnapshotFanLX> snapshot() const {
        std::vector<GroupMemberSnapshotFanLX> result;
        result.reserve(stateFanLX.members.size());
        for (const auto &member : stateFanLX.members)
            result.push_back({member.first, member.second.role, member.second.joinOrder});
        std::sort(result.begin(), result.end(),
                  [](const auto &left, const auto &right) { return left.id < right.id; });
        return result;
    }
    void stampNewMembers(const std::string &time, std::size_t fromOrder) {
        for (auto &item : stateFanLX.members)
            if (item.second.joinOrder >= fromOrder && !item.second.joinedAtUtc)
                item.second.joinedAtUtc = time;
    }
    std::size_t nextJoinOrder() const noexcept {
        return stateFanLX.nextJoinOrder;
    }
    const std::string &name() const noexcept {
        return nameFanLX;
    }
    void rename(std::string name) {
        nameFanLX = std::move(name);
    }
    bool canCreateDiscussion() const {
        return policyFanLX->canCreateDiscussion();
    }
    // 群主不能直接离开，以免产生无人管理的群。
    bool leave(const std::string &userId) {
        if (!hasMember(userId))
            return false;
        if (roleOf(userId) == GroupRoleFanLX::Owner)
            throw std::runtime_error("群主退群前必须转让群主");
        stateFanLX.members.erase(userId);
        return true;
    }

    void transferOwnership(const std::string &newOwner) {
        auto target = stateFanLX.members.find(newOwner);
        if (target == stateFanLX.members.end())
            throw std::out_of_range("新群主必须是现有成员");
        // 先完成存在性检查，再修改角色；失败不会留下半转让状态。
        for (auto &member : stateFanLX.members)
            if (member.second.role == GroupRoleFanLX::Owner)
                member.second.role = GroupRoleFanLX::Member;
        target->second.role = GroupRoleFanLX::Owner;
    }
    std::vector<std::string> members() const {
        std::vector<std::string> out;
        for (const auto &m : stateFanLX.members)
            out.push_back(m.first);
        std::sort(out.begin(), out.end());
        return out;
    }
};
