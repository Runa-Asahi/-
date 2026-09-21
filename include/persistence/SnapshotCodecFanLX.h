#pragma once
#include "persistence/PlatformSnapshotFanLX.h"
#include "persistence/PlatformStateFanLX.h"
#include <nlohmann/json.hpp>

// 恢复函数只接收通过 validate 的 DTO；在线状态由应用私有持有。
class SnapshotCodecFanLX {
  public:
    static PlatformSnapshotFanLX capture(const PlatformStateFanLX &state);
    static PlatformStateFanLX build(const PlatformSnapshotFanLX &snapshot);
    static nlohmann::json encode(const PlatformSnapshotFanLX &snapshot);
    static PlatformSnapshotFanLX decode(const nlohmann::json &json);
    static PlatformSnapshotFanLX parse(const std::string &bytes);
    static void copySessions(PlatformStateFanLX &to, const PlatformStateFanLX &from);

  private:
    static GroupRecordFanLX captureGroup(const GroupServiceFanLX &groups, const GroupKeyFanLX &key);
    static void restoreGroup(GroupServiceFanLX &groups, const GroupRecordFanLX &record);
};
