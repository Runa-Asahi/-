#pragma once
#include "persistence/PlatformStateFanLX.h"

// 准备阶段完成全部查找/分配。publish 不查哈希、不创建对象、不执行用户回调。
class PreparedCommitFanLX {
    PlatformStateFanLX &liveFanLX;
    PlatformStateFanLX &candidateFanLX;
    std::vector<std::pair<GroupFanLX *, GroupFanLX *>> groupsFanLX;
    std::vector<std::pair<DiscussionGroupFanLX *, DiscussionGroupFanLX *>> discussionsFanLX;
    std::vector<std::shared_ptr<GroupFanLX>> heldGroupsFanLX;
    std::vector<std::shared_ptr<DiscussionGroupFanLX>> heldDiscussionsFanLX;

  public:
    PreparedCommitFanLX(PlatformStateFanLX &live, PlatformStateFanLX &candidate);
    void publish() noexcept;
};
