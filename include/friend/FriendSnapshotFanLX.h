#pragma once
#include "account/AccountRefFanLX.h"
struct FriendSnapshotFanLX {
    AccountRefFanLX left;
    AccountRefFanLX right;
    std::string leftRemark, leftTag, rightRemark, rightTag;
};
