#pragma once
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ServiceTypeFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
enum class ServiceTypeFanLX {
    QQ,     // 腾讯 QQ
    Wechat, // 微信
    Weibo   // 新浪微博
};
inline std::string serviceNameFanLX(ServiceTypeFanLX type) {
    switch (type) {
    case ServiceTypeFanLX::QQ:
        return "QQ";
    case ServiceTypeFanLX::Wechat:
        return "Wechat";
    default:
        return "Weibo";
    }
}
