#pragma once
#include "account/ServiceAccountFanLX.h"
#include "account/ServiceNotConfirmedFanLX.h"
#include "account/ServiceNotSubscribedFanLX.h"
#include "account/SessionExpiredFanLX.h"
#include "account/UserProfileFanLX.h"
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

// AccountManagerFanLX：从原领域聚合头文件拆出，保留已验证的业务接口。
class AccountSessionFanLX;
class AccountManagerFanLX {
    friend class SnapshotCodecFanLX;
    friend class PreparedCommitFanLX;
    // 存储键 = 用户ID + "|" + 服务类型
    std::unordered_map<std::string, UserProfileFanLX> usersFanLX;
    std::unordered_map<std::string, ServiceAccountFanLX> accountsFanLX;
    // 平台账号索引：键 "服务名|账号ID" -> 账号存储键。
    // accountId 是前端登录身份，必须平台内唯一；该索引同时承担唯一性校验与 O(1) 定位。
    std::unordered_map<std::string, std::string> accountIdIndexFanLX;
    std::unordered_set<std::string> subscriptionsFanLX; // 已"开通"服务的键集合（键格式 "userId|服务名"）
    std::unordered_map<std::string, std::string> bindingsFanLX; // 绑定关系：账号键 -> 与之绑定的另一账号键
    std::unordered_set<std::string> loggedInFanLX;              // 当前处于"登录态"的账号键集合

    // 工具函数：由 用户ID + 服务类型 生成统一存储键，如 "U001|QQ"。
    static std::string keyFanLX(const std::string &user, ServiceTypeFanLX service) {
        return user + "|" + serviceNameFanLX(service);
    }
    // 工具函数：由 平台 + 账号 ID 生成索引键，如 "QQ|20260001"。
    static std::string accountIdKeyFanLX(ServiceTypeFanLX service, const std::string &accountId) {
        return serviceNameFanLX(service) + "|" + accountId;
    }
    // 工具函数：由 平台 + 账号 ID 定位账号存储键；账号不存在时抛异常。
    std::string keyOfAccountIdFanLX(ServiceTypeFanLX service, const std::string &accountId) const {
        auto it = accountIdIndexFanLX.find(accountIdKeyFanLX(service, accountId));
        if (it == accountIdIndexFanLX.end())
            throw std::runtime_error("平台账号不存在");
        return it->second;
    }
    // 工具函数：删除一条指定方向的绑定记录，返回是否真的删除了。
    bool dropBindingFanLX(const std::string &fromKey, const std::string &toKey) {
        auto it = bindingsFanLX.find(fromKey);
        if (it == bindingsFanLX.end() || it->second != toKey)
            return false;
        bindingsFanLX.erase(it);
        return true;
    }

    // 工具函数：判断两个账号键是否"同号"——双方都标记了 sharedId，且账号 ID 完全相同。
    // 语义：同一个账号 ID 在多个平台复用，因此一次登录应当在这些平台同时生效。
    bool sameSharedAccountFanLX(const std::string &leftKey, const std::string &rightKey) const {
        const auto &left = accountsFanLX.at(leftKey), &right = accountsFanLX.at(rightKey);
        return left.sharedId && right.sharedId && left.accountId == right.accountId;
    }

    // 工具函数：判断两个账号键之间是否存在显式绑定关系（双向查 bindingsFanLX）。
    // 语义：平台不同、账号 ID 也完全不同（如 QQ 号与微信号），但用户主动建立过绑定，
    //       这是统一登录的第二个依据，也是 bindingsFanLX 真正的读者。
    bool boundFanLX(const std::string &leftKey, const std::string &rightKey) const {
        auto it = bindingsFanLX.find(leftKey);
        if (it != bindingsFanLX.end() && it->second == rightKey)
            return true;
        it = bindingsFanLX.find(rightKey);
        return it != bindingsFanLX.end() && it->second == leftKey;
    }

    // 登录核心：把 currentKey 置为登录态；confirmOthers 时联动该用户其它已开通平台。
    // user 仅用于把遍历范围限定在"同一用户的订阅键"上，不构成登录依据。
    void loginCoreFanLX(const std::string &user, const std::string &currentKey, bool confirmOthers) {
        // 未开通的服务不允许登录（工程计划：未开通服务不被自动开通）
        if (!subscriptionsFanLX.count(currentKey))
            throw ServiceNotSubscribedFanLX("服务尚未开通");
        loggedInFanLX.insert(currentKey);
        if (!confirmOthers)
            return;
        for (const auto &item : subscriptionsFanLX) // 只遍历已开通的平台
        {
            if (item == currentKey || item.rfind(user + "|", 0) != 0) // 跳过自身与他人的订阅键
                continue;
            // 只有"同号"或"显式绑定"才联动；"同一个人"本身不是联动理由
            if (sameSharedAccountFanLX(item, currentKey) || boundFanLX(item, currentKey))
                loggedInFanLX.insert(item);
        }
    }

  public:
    // ==================== 后端配给 API（以内部 userId 为键，前端不调用） ====================

    // 注册新用户：用户 ID 必须全局唯一，重复注册抛出异常。
    void addUser(UserProfileFanLX profile) {
        // emplace 失败（键已存在）返回 second=false
        if (!usersFanLX.emplace(profile.userId, std::move(profile)).second)
            throw std::runtime_error("用户ID已存在");
    }

    // 为某用户建号（开通平台账号）：用户必须已注册；
    // 同一用户在同一平台只能有一个账号，且同一平台的账号 ID 不允许被重复注册
    // —— accountId 是前端登录身份，必须平台内唯一，否则登录无法唯一定位。
    void addAccount(ServiceAccountFanLX account) {
        // 校验归属用户是否存在
        if (usersFanLX.count(account.userId) == 0)
            throw std::runtime_error("账号归属用户不存在");
        // 唯一性 1：同一用户在同一平台只能有一个账号
        auto key = keyFanLX(account.userId, account.service);
        if (accountsFanLX.count(key) != 0)
            throw std::runtime_error("服务账号已存在");
        // 唯一性 2：同一平台的账号 ID 不能被两人共用（否则登录身份不唯一）
        auto index = accountIdKeyFanLX(account.service, account.accountId);
        if (accountIdIndexFanLX.count(index) != 0)
            throw std::runtime_error("该平台账号已被占用");
        accountIdIndexFanLX.emplace(index, key); // 先登记索引，再登记账号
        accountsFanLX.emplace(key, std::move(account));
    }

    // 开通服务（后端配给口径）：前提是该用户在该平台已存在服务账号。
    // 前端用户"自主开通"请用 AccountSessionFanLX::subscribeTo。
    void subscribe(const std::string &user, ServiceTypeFanLX service) {
        if (accountsFanLX.count(keyFanLX(user, service)) == 0)
            throw std::runtime_error("该服务账号不存在");
        subscriptionsFanLX.insert(keyFanLX(user, service));
    }

    void updateProfile(const std::string &user, std::string nickname, std::string location) {
        auto &profile = usersFanLX.at(user);
        profile.nickname = std::move(nickname);
        profile.location = std::move(location);
    }

    // ==================== 前端入口：登录（签发会话） ====================

    // 以「平台账号」登录并签发会话：调用方持有的是 QQ 号 / 微信号，不涉及内部 userId。
    // confirmOthers 为 true 时，按"同号 / 显式绑定"两条依据批量确认登录该用户其它已开通平台
    // （对应工程计划"一次简单确认登录全部"）；为 false 则只登录当前账号。
    // 返回的会话是前端身份载体，只有服务与账号 ID，拿不到 userId。
    AccountSessionFanLX login(ServiceTypeFanLX service, const std::string &accountId, bool confirmOthers);

    // ==================== 后端查询 API（配给、统计、持久化与测试用） ====================

    // 查询：用户是否已开通某平台服务。
    bool isSubscribed(const std::string &user, ServiceTypeFanLX service) const {
        return subscriptionsFanLX.count(keyFanLX(user, service)) != 0;
    }
    // 查询：用户在某平台是否处于登录态。
    bool isLoggedIn(const std::string &user, ServiceTypeFanLX service) const {
        return loggedInFanLX.count(keyFanLX(user, service)) != 0;
    }
    // 查询：取出用户资料（键不存在时 at 会抛出异常）。
    const UserProfileFanLX &profile(const std::string &user) const {
        return usersFanLX.at(user);
    }
    // 查询：按「归属用户 + 平台」找服务账号；不存在时返回 nullptr（"查不到就跳过"的内部换算用）。
    const ServiceAccountFanLX *findAccount(const std::string &user, ServiceTypeFanLX service) const {
        auto it = accountsFanLX.find(keyFanLX(user, service));
        return it == accountsFanLX.end() ? nullptr : &it->second;
    }
    // 查询：按「归属用户 + 平台」取服务账号；不存在时抛异常。
    const ServiceAccountFanLX &account(const std::string &user, ServiceTypeFanLX service) const {
        const auto *found = findAccount(user, service);
        if (!found)
            throw std::runtime_error("服务账号不存在");
        return *found;
    }

    // 查询：列出某用户拥有的全部服务账号。
    std::vector<ServiceAccountFanLX> accounts(const std::string &user) const {
        std::vector<ServiceAccountFanLX> result;
        // 遍历所有账号，筛选出归属该用户的（结构化绑定解出 键-值 对）
        for (const auto &[key, a] : accountsFanLX)
            if (a.userId == user)
                result.push_back(a);
        return result;
    }

    // ==================== 平台账号视角的操作（供会话与好友域使用） ====================

    // 查询：某平台账号是否已建号（不抛异常）。
    bool hasAccount(ServiceTypeFanLX service, const std::string &accountId) const {
        return accountIdIndexFanLX.count(accountIdKeyFanLX(service, accountId)) != 0;
    }
    // 查询：某平台账号是否已开通该平台服务（账号不存在时返回 false，不抛异常）。
    bool isAccountSubscribed(ServiceTypeFanLX service, const std::string &accountId) const {
        auto it = accountIdIndexFanLX.find(accountIdKeyFanLX(service, accountId));
        return it != accountIdIndexFanLX.end() && subscriptionsFanLX.count(it->second) != 0;
    }
    // 查询：某平台账号是否处于登录态（账号不存在时返回 false，不抛异常）。
    bool isAccountLoggedIn(ServiceTypeFanLX service, const std::string &accountId) const {
        auto it = accountIdIndexFanLX.find(accountIdKeyFanLX(service, accountId));
        return it != accountIdIndexFanLX.end() && loggedInFanLX.count(it->second) != 0;
    }
    // 查询：按「平台 + 账号 ID」取服务账号（不存在时抛"平台账号不存在"）。
    const ServiceAccountFanLX &accountByAccountId(ServiceTypeFanLX service,
                                                  const std::string &accountId) const {
        return accountsFanLX.at(keyOfAccountIdFanLX(service, accountId));
    }

    // 注销当前平台服务：只清除该账号自身登录态，不联动其它平台。
    void logout(ServiceTypeFanLX service, const std::string &accountId) {
        loggedInFanLX.erase(keyOfAccountIdFanLX(service, accountId));
    }

    // 平台注销：退出该账号所属用户在**全部平台**的登录态（对应工程计划"平台注销"）。
    // userId 只在后端参与匹配，前端通过会话调用，签名里不出现 userId。
    void logoutAll(ServiceTypeFanLX service, const std::string &accountId) {
        const std::string &user = accountsFanLX.at(keyOfAccountIdFanLX(service, accountId)).userId;
        for (auto it = loggedInFanLX.begin(); it != loggedInFanLX.end();) {
            if (it->rfind(user + "|", 0) == 0)
                it = loggedInFanLX.erase(it); // 属于该用户的登录记录：删除并推进迭代器
            else
                ++it;
        }
    }

    // 建立绑定：把两个平台账号关联起来，参数全是平台账号，前端无需 userId。
    // 工程计划 §4.2：绑定双方必须属于同一平台用户、目标账号存在且归属可验证，
    // 不允许"输入任意 QQ 号就把别人的账号绑到自己名下"。
    void bindAccounts(ServiceTypeFanLX fromService, const std::string &fromAccountId,
                      ServiceTypeFanLX toService, const std::string &toAccountId) {
        if (fromService == toService)
            throw std::runtime_error("不支持绑定同一平台的两个账号");
        // 两个账号都必须已建号（否则 keyOfAccountIdFanLX 抛"平台账号不存在"）
        auto fromKey = keyOfAccountIdFanLX(fromService, fromAccountId);
        auto toKey = keyOfAccountIdFanLX(toService, toAccountId);
        // 归属校验：只有同一内部用户的两个账号才能互相绑定
        if (accountsFanLX.at(fromKey).userId != accountsFanLX.at(toKey).userId)
            throw std::runtime_error("绑定双方必须属于同一用户");
        bindingsFanLX[fromKey] = toKey; // 记录 from -> to 的绑定关系
    }

    // 解除绑定：删除成对的绑定记录，返回是否真的删除了一条。
    // 解绑后不再参与统一登录联动，也不再参与任何依赖绑定映射的跨服务匹配。
    bool unbindAccounts(ServiceTypeFanLX fromService, const std::string &fromAccountId,
                        ServiceTypeFanLX toService, const std::string &toAccountId) {
        auto fromKey = keyOfAccountIdFanLX(fromService, fromAccountId);
        auto toKey = keyOfAccountIdFanLX(toService, toAccountId);
        // 两个方向都尝试删除，解绑对调用方的书写方向不敏感
        bool removed = dropBindingFanLX(fromKey, toKey);
        return dropBindingFanLX(toKey, fromKey) || removed;
    }

    // 查询：两个平台账号之间是否存在绑定关系（双向判定）。
    bool isBound(ServiceTypeFanLX fromService, const std::string &fromAccountId, ServiceTypeFanLX toService,
                 const std::string &toAccountId) const {
        return boundFanLX(keyOfAccountIdFanLX(fromService, fromAccountId),
                          keyOfAccountIdFanLX(toService, toAccountId));
    }

    // 查询：某平台账号的全部绑定对端账号（按账号键排序，保证输出稳定可测）。
    std::vector<ServiceAccountFanLX> bindingsOf(ServiceTypeFanLX service,
                                                const std::string &accountId) const {
        const auto self = keyOfAccountIdFanLX(service, accountId);
        std::vector<std::string> peers; // 先收集对端账号键
        auto it = bindingsFanLX.find(self);
        if (it != bindingsFanLX.end())
            peers.push_back(it->second); // 本账号指向别人
        for (const auto &[from, to] : bindingsFanLX)
            if (to == self)
                peers.push_back(from); // 别人指向本账号
        std::sort(peers.begin(), peers.end());
        std::vector<ServiceAccountFanLX> result;
        for (const auto &peer : peers)
            result.push_back(accountsFanLX.at(peer));
        return result;
    }
};
#include "account/AccountSessionFanLX.h"
