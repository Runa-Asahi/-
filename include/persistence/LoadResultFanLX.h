#pragma once
#include "persistence/PlatformSnapshotFanLX.h"
enum class LoadStatusFanLX { Empty, Loaded, RecoveredReadOnly, UnsupportedSchema, UncommittedOnly, Failed };
struct LoadResultFanLX {
    LoadStatusFanLX status = LoadStatusFanLX::Empty;
    PlatformSnapshotFanLX snapshot;
    std::string message;
};
