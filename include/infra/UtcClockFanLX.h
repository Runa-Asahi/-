#pragma once
#include <ctime>
#include <stdexcept>
#include <string>

// 事务开始时取一次时间；应用可注入固定时钟以获得确定性夹具。
inline std::string utcNowFanLX() {
    const std::time_t now = std::time(nullptr);
    std::tm value{};
    if (gmtime_s(&value, &now) != 0)
        throw std::runtime_error("无法读取 UTC 时间");
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &value);
    return buffer;
}
