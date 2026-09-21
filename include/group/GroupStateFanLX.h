#pragma once
#include "group/GroupMembershipFanLX.h"
#include <string>
#include <unordered_map>

// 群对象的可变数据；发布只交换内部状态，不替换外壳。
struct GroupStateFanLX {
    std::unordered_map<std::string, GroupMembershipFanLX> members;
    std::size_t nextJoinOrder = 0;
    void swap(GroupStateFanLX &other) noexcept {
        static_assert(noexcept(members.swap(other.members)), "成员容器交换必须不抛异常");
        members.swap(other.members);
        std::swap(nextJoinOrder, other.nextJoinOrder);
    }
};
