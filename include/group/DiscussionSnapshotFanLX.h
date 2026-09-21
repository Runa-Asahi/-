#pragma once
#include <string>
#include <vector>
struct DiscussionSnapshotFanLX {
    std::string id;
    std::vector<std::string> members;
    std::string owner;
    bool archived = false;
};
