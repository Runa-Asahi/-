#pragma once
// 群快照记录：持久化用的完整群状态（身份、当前模式、成员状态、三张工作流与子群）。
#include "group/DiscussionSnapshotFanLX.h"
#include "group/GroupApplicationFanLX.h"
#include "group/GroupInvitationFanLX.h"
#include "group/GroupKeyFanLX.h"
#include "group/GroupModeFanLX.h"
#include "group/GroupStateFanLX.h"
#include <string>
#include <vector>
struct GroupRecordFanLX {
    GroupKeyFanLX key;
    GroupModeFanLX currentMode;
    GroupStateFanLX state;
    std::vector<GroupApplicationFanLX> applications;
    std::vector<GroupInvitationFanLX> invitations;
    std::vector<DiscussionSnapshotFanLX> discussions;
    std::string name;
};
