#pragma once
#include "persistence/PlatformSnapshotFanLX.h"
class SchemaValidatorFanLX {
  public:
    static void validate(const PlatformSnapshotFanLX &snapshot);
};
