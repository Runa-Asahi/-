#pragma once
// 账号引用：以「平台 + 平台账号 ID」跳域引用一个服务账号，不含归属用户信息。
#include "account/ServiceTypeFanLX.h"
#include <string>
struct AccountRefFanLX {
    ServiceTypeFanLX service;
    std::string accountId;
};
