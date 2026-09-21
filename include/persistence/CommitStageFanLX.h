#pragma once
#include <functional>
enum class CommitStageFanLX {
    BeforeWrite,
    DuringWrite,
    BeforeFlush,
    AfterTemporary,
    BeforeBackup,
    AfterBackup,
    BeforePublish,
    AfterPublish,
    BeforeMemoryPublish,
    BeforeReply
};
using CommitHookFanLX = std::function<void(CommitStageFanLX)>;
