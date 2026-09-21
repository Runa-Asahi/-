#include "friend/IdentityResolverFanLX.h"
#include "persistence/PersistenceErrorFanLX.h"
#include "persistence/SchemaValidatorFanLX.h"
#include "persistence/SnapshotCodecFanLX.h"
#include <map>
#include <set>

namespace {
using JsonFanLX = nlohmann::json;
[[noreturn]] void invalidFanLX(const std::string &message) {
    throw PersistenceErrorFanLX(PersistenceErrorCodeFanLX::InvalidData, message);
}
std::string modeNameFanLX(GroupModeFanLX m) {
    return m == GroupModeFanLX::QQ ? "QQ" : "Wechat";
}
GroupModeFanLX modeFanLX(const JsonFanLX &j) {
    if (j == "QQ")
        return GroupModeFanLX::QQ;
    if (j == "Wechat")
        return GroupModeFanLX::Wechat;
    invalidFanLX("未知群模式");
}
ServiceTypeFanLX serviceFanLX(const JsonFanLX &j) {
    if (j == "QQ")
        return ServiceTypeFanLX::QQ;
    if (j == "Wechat")
        return ServiceTypeFanLX::Wechat;
    if (j == "Weibo")
        return ServiceTypeFanLX::Weibo;
    invalidFanLX("未知服务");
}
std::string roleNameFanLX(GroupRoleFanLX r) {
    if (r == GroupRoleFanLX::Owner)
        return "Owner";
    if (r == GroupRoleFanLX::Admin)
        return "Admin";
    return "Member";
}
GroupRoleFanLX roleFanLX(const JsonFanLX &j) {
    if (j == "Owner")
        return GroupRoleFanLX::Owner;
    if (j == "Admin")
        return GroupRoleFanLX::Admin;
    if (j == "Member")
        return GroupRoleFanLX::Member;
    invalidFanLX("未知角色");
}
std::string strFanLX(const JsonFanLX &j) {
    if (!j.is_string())
        invalidFanLX("字段必须是字符串");
    auto s = j.get<std::string>();
    if (s.size() > 4096)
        invalidFanLX("字符串超出限制");
    return s;
}
std::uint64_t uintFanLX(const JsonFanLX &j) {
    if (!j.is_number_unsigned())
        invalidFanLX("字段必须是无符号整数");
    return j.get<std::uint64_t>();
}
bool boolFanLX(const JsonFanLX &j) {
    if (!j.is_boolean())
        invalidFanLX("字段必须是布尔值");
    return j.get<bool>();
}
const JsonFanLX &arrayFanLX(const JsonFanLX &j, std::size_t max = 1000000) {
    if (!j.is_array() || j.size() > max)
        invalidFanLX("数组类型或数量不合法");
    return j;
}
std::optional<std::string> optionalFanLX(const JsonFanLX &j) {
    return j.is_null() ? std::nullopt : std::optional<std::string>(strFanLX(j));
}
JsonFanLX optionalJsonFanLX(const std::optional<std::string> &s) {
    return s ? JsonFanLX(*s) : JsonFanLX(nullptr);
}
JsonFanLX refJsonFanLX(const AccountRefFanLX &r) {
    return {{"service", serviceNameFanLX(r.service)}, {"accountId", r.accountId}};
}
AccountRefFanLX refFanLX(const JsonFanLX &j) {
    return {serviceFanLX(j.at("service")), strFanLX(j.at("accountId"))};
}
JsonFanLX groupJsonFanLX(const GroupRecordFanLX &g) {
    JsonFanLX j = {{"key", {{"namespaceMode", modeNameFanLX(g.key.mode)}, {"id", g.key.id}}},
                   {"currentMode", modeNameFanLX(g.currentMode)},
                   {"name", g.name},
                   {"nextJoinOrder", g.state.nextJoinOrder},
                   {"members", JsonFanLX::array()},
                   {"applications", JsonFanLX::array()},
                   {"invitations", JsonFanLX::array()},
                   {"discussions", JsonFanLX::array()}};
    std::map<std::string, GroupMembershipFanLX> sorted(g.state.members.begin(), g.state.members.end());
    for (const auto &m : sorted)
        j["members"].push_back({{"userId", m.first},
                                {"role", roleNameFanLX(m.second.role)},
                                {"joinOrder", m.second.joinOrder},
                                {"joinedAtUtc", optionalJsonFanLX(m.second.joinedAtUtc)}});
    for (const auto &a : g.applications)
        j["applications"].push_back({{"applicant", a.applicant},
                                     {"status", a.status},
                                     {"createdAtUtc", optionalJsonFanLX(a.createdAtUtc)},
                                     {"expiresAtUtc", optionalJsonFanLX(a.expiresAtUtc)}});
    for (const auto &a : g.invitations)
        j["invitations"].push_back({{"inviter", a.inviter},
                                    {"invitee", a.invitee},
                                    {"status", a.status},
                                    {"createdAtUtc", optionalJsonFanLX(a.createdAtUtc)},
                                    {"expiresAtUtc", optionalJsonFanLX(a.expiresAtUtc)}});
    for (const auto &d : g.discussions)
        j["discussions"].push_back(
            {{"id", d.id}, {"members", d.members}, {"owner", d.owner}, {"archived", d.archived}});
    return j;
}
GroupRecordFanLX groupFanLX(const JsonFanLX &j) {
    GroupRecordFanLX g;
    g.key = {modeFanLX(j.at("key").at("namespaceMode")), strFanLX(j.at("key").at("id"))};
    g.currentMode = modeFanLX(j.at("currentMode"));
    g.name = strFanLX(j.at("name"));
    const auto next = uintFanLX(j.at("nextJoinOrder"));
    if (next > std::numeric_limits<std::size_t>::max())
        invalidFanLX("序号超出平台范围");
    g.state.nextJoinOrder = static_cast<std::size_t>(next);
    for (const auto &m : arrayFanLX(j.at("members"), 10000)) {
        auto n = uintFanLX(m.at("joinOrder"));
        if (n > std::numeric_limits<std::size_t>::max())
            invalidFanLX("序号超出平台范围");
        if (!g.state.members
                 .emplace(strFanLX(m.at("userId")),
                          GroupMembershipFanLX{roleFanLX(m.at("role")), static_cast<std::size_t>(n),
                                               optionalFanLX(m.at("joinedAtUtc"))})
                 .second)
            invalidFanLX("重复群成员");
    }
    for (const auto &a : arrayFanLX(j.at("applications"), 10000)) {
        auto status = strFanLX(a.at("status"));
        g.applications.push_back({strFanLX(a.at("applicant")), status == "Pending", status,
                                  optionalFanLX(a.at("createdAtUtc")), optionalFanLX(a.at("expiresAtUtc"))});
    }
    for (const auto &a : arrayFanLX(j.at("invitations"), 10000)) {
        auto status = strFanLX(a.at("status"));
        g.invitations.push_back({strFanLX(a.at("inviter")), strFanLX(a.at("invitee")), status == "Pending",
                                 status, optionalFanLX(a.at("createdAtUtc")),
                                 optionalFanLX(a.at("expiresAtUtc"))});
    }
    for (const auto &d : arrayFanLX(j.at("discussions"), 10000)) {
        DiscussionSnapshotFanLX record{
            strFanLX(d.at("id")), {}, strFanLX(d.at("owner")), boolFanLX(d.at("archived"))};
        for (const auto &m : arrayFanLX(d.at("members"), 10000))
            record.members.push_back(strFanLX(m));
        g.discussions.push_back(std::move(record));
    }
    return g;
}
} // namespace

GroupRecordFanLX SnapshotCodecFanLX::captureGroup(const GroupServiceFanLX &service,
                                                  const GroupKeyFanLX &key) {
    const auto &g = service.get(key);
    GroupRecordFanLX result{key, g.currentMode(), g.stateFanLX, {}, {}, {}, g.nameFanLX};
    auto a = service.applicationsFanLX.find(key);
    if (a != service.applicationsFanLX.end())
        for (const auto &item : a->second)
            result.applications.push_back(item.second);
    auto i = service.invitationsFanLX.find(key);
    if (i != service.invitationsFanLX.end())
        for (const auto &item : i->second)
            result.invitations.push_back(item.second);
    auto d = service.discussionsFanLX.find(key);
    if (d != service.discussionsFanLX.end())
        for (const auto &item : d->second) {
            const auto &child = *item.second;
            DiscussionSnapshotFanLX record{child.idFanLX, {}, child.ownerFanLX, child.archivedFanLX};
            record.members.assign(child.membersFanLX.begin(), child.membersFanLX.end());
            std::sort(record.members.begin(), record.members.end());
            result.discussions.push_back(std::move(record));
        }
    std::sort(result.applications.begin(), result.applications.end(),
              [](const auto &l, const auto &r) { return l.applicant < r.applicant; });
    std::sort(result.invitations.begin(), result.invitations.end(),
              [](const auto &l, const auto &r) { return l.invitee < r.invitee; });
    std::sort(result.discussions.begin(), result.discussions.end(),
              [](const auto &l, const auto &r) { return l.id < r.id; });
    return result;
}
PlatformSnapshotFanLX SnapshotCodecFanLX::capture(const PlatformStateFanLX &state) {
    PlatformSnapshotFanLX s;
    s.generation = state.generation;
    s.parentGeneration = state.parentGeneration;
    s.committedAtUtc = state.committedAtUtc;
    s.versions = state.versions;
    const auto &a = state.accounts;
    for (const auto &item : a.usersFanLX)
        s.users.push_back(item.second);
    std::map<std::string, AccountRefFanLX> refs;
    for (const auto &item : a.accountsFanLX) {
        const auto &account = item.second;
        s.accounts.push_back(account);
        AccountRefFanLX ref{account.service, account.accountId};
        refs.emplace(account.userId + "|" + serviceNameFanLX(account.service) + "|" + account.accountId, ref);
        if (a.subscriptionsFanLX.count(item.first))
            s.subscriptions.push_back(ref);
    }
    // 绑定方向原样保存，避免一对多关系被导入时覆写。
    for (const auto &item : a.bindingsFanLX) {
        const auto &l = a.accountsFanLX.at(item.first);
        const auto &r = a.accountsFanLX.at(item.second);
        s.bindings.push_back({{l.service, l.accountId}, {r.service, r.accountId}});
    }
    for (const auto &item : state.friends.detailsFanLX) {
        const auto &f = item.second;
        if (f.ownerAccountKey < f.peerAccountKey) {
            auto reverse = state.friends.detail(f.peerAccountKey, f.ownerAccountKey);
            s.friendships.push_back({refs.at(f.ownerAccountKey), refs.at(f.peerAccountKey), f.remark, f.tag,
                                     reverse.remark, reverse.tag});
        }
    }
    for (const auto &g : state.groups.groupsFanLX)
        s.groups.push_back(captureGroup(state.groups, g.first));
    for (const auto &archive : state.groups.archivesFanLX)
        for (const auto &g : archive->groupsFanLX)
            s.archivedGroups.push_back(captureGroup(*archive, g.first));
    auto accountLess = [](const auto &l, const auto &r) {
        return std::make_pair(l.service, l.accountId) < std::make_pair(r.service, r.accountId);
    };
    std::sort(s.users.begin(), s.users.end(),
              [](const auto &l, const auto &r) { return l.userId < r.userId; });
    std::sort(s.accounts.begin(), s.accounts.end(), accountLess);
    std::sort(s.subscriptions.begin(), s.subscriptions.end(), accountLess);
    std::sort(s.bindings.begin(), s.bindings.end(),
              [&](const auto &l, const auto &r) { return accountLess(l.left, r.left); });
    std::sort(s.friendships.begin(), s.friendships.end(), [&](const auto &l, const auto &r) {
        return std::make_tuple(l.left.service, l.left.accountId, l.right.accountId) <
               std::make_tuple(r.left.service, r.left.accountId, r.right.accountId);
    });
    std::sort(s.groups.begin(), s.groups.end(), [](const auto &l, const auto &r) {
        return std::make_pair(l.key.mode, l.key.id) < std::make_pair(r.key.mode, r.key.id);
    });
    return s;
}
void SnapshotCodecFanLX::restoreGroup(GroupServiceFanLX &service, const GroupRecordFanLX &r) {
    auto &g = service.create(r.key);
    if (r.currentMode == GroupModeFanLX::QQ)
        g.switchPolicy(std::make_unique<QQGroupPolicyFanLX>());
    else
        g.switchPolicy(std::make_unique<WechatGroupPolicyFanLX>());
    g.stateFanLX = r.state;
    g.nameFanLX = r.name;
    for (const auto &a : r.applications)
        service.applicationsFanLX[r.key].emplace(a.applicant, a);
    for (const auto &a : r.invitations)
        service.invitationsFanLX[r.key].emplace(a.invitee, a);
    for (const auto &d : r.discussions) {
        auto child = std::make_shared<DiscussionGroupFanLX>(d.id);
        child->membersFanLX.insert(d.members.begin(), d.members.end());
        child->ownerFanLX = d.owner;
        child->archivedFanLX = d.archived;
        service.discussionsFanLX[r.key].emplace(d.id, std::move(child));
    }
}
PlatformStateFanLX SnapshotCodecFanLX::build(const PlatformSnapshotFanLX &s) {
    SchemaValidatorFanLX::validate(s);
    PlatformStateFanLX state;
    state.generation = s.generation;
    state.parentGeneration = s.parentGeneration;
    state.committedAtUtc = s.committedAtUtc;
    state.versions = s.versions;
    for (const auto &u : s.users)
        state.accounts.addUser(u);
    for (const auto &a : s.accounts)
        state.accounts.addAccount(a);
    for (const auto &r : s.subscriptions)
        state.accounts.subscribe(state.accounts.accountByAccountId(r.service, r.accountId).userId, r.service);
    for (const auto &b : s.bindings)
        state.accounts.bindAccounts(b.left.service, b.left.accountId, b.right.service, b.right.accountId);
    IdentityResolverFanLX resolver(state.accounts);
    for (const auto &f : s.friendships) {
        auto l = resolver.accountKey(f.left.service, f.left.accountId),
             r = resolver.accountKey(f.right.service, f.right.accountId);
        state.friends.add(l, r);
        state.friends.update(l, r, f.leftRemark, f.leftTag);
        state.friends.update(r, l, f.rightRemark, f.rightTag);
    }
    for (const auto &g : s.groups)
        restoreGroup(state.groups, g);
    for (const auto &g : s.archivedGroups) {
        auto service = std::make_shared<GroupServiceFanLX>();
        restoreGroup(*service, g);
        state.groups.archivesFanLX.push_back(std::move(service));
    }
    return state;
}
nlohmann::json SnapshotCodecFanLX::encode(const PlatformSnapshotFanLX &s) {
    JsonFanLX j = {{"schemaVersion", 1u},
                   {"generation", s.generation},
                   {"parentGeneration", s.parentGeneration},
                   {"committedAtUtc", s.committedAtUtc},
                   {"versions", s.versions}};
    for (const auto &key :
         {"users", "accounts", "subscriptions", "bindings", "friendships", "groups", "archivedGroups"})
        j[key] = JsonFanLX::array();
    for (const auto &u : s.users)
        j["users"].push_back({{"userId", u.userId},
                              {"nickname", u.nickname},
                              {"birthDate", u.birthDate},
                              {"applyDate", u.applyDate},
                              {"location", u.location}});
    for (const auto &a : s.accounts)
        j["accounts"].push_back({{"userId", a.userId},
                                 {"service", serviceNameFanLX(a.service)},
                                 {"accountId", a.accountId},
                                 {"sharedId", a.sharedId}});
    for (const auto &r : s.subscriptions)
        j["subscriptions"].push_back(refJsonFanLX(r));
    for (const auto &b : s.bindings)
        j["bindings"].push_back({{"left", refJsonFanLX(b.left)}, {"right", refJsonFanLX(b.right)}});
    for (const auto &f : s.friendships)
        j["friendships"].push_back({{"left", refJsonFanLX(f.left)},
                                    {"right", refJsonFanLX(f.right)},
                                    {"leftRemark", f.leftRemark},
                                    {"leftTag", f.leftTag},
                                    {"rightRemark", f.rightRemark},
                                    {"rightTag", f.rightTag}});
    for (const auto &g : s.groups)
        j["groups"].push_back(groupJsonFanLX(g));
    for (const auto &g : s.archivedGroups)
        j["archivedGroups"].push_back(groupJsonFanLX(g));
    return j;
}
PlatformSnapshotFanLX SnapshotCodecFanLX::decode(const nlohmann::json &j) {
    try {
        if (uintFanLX(j.at("schemaVersion")) != 1)
            throw PersistenceErrorFanLX(PersistenceErrorCodeFanLX::UnsupportedSchema, "未知 schemaVersion");
        PlatformSnapshotFanLX s;
        s.generation = uintFanLX(j.at("generation"));
        s.parentGeneration = uintFanLX(j.at("parentGeneration"));
        s.committedAtUtc = strFanLX(j.at("committedAtUtc"));
        if (!j.at("versions").is_object())
            invalidFanLX("版本表须为对象");
        for (auto it = j.at("versions").begin(); it != j.at("versions").end(); ++it)
            s.versions.emplace(it.key(), uintFanLX(it.value()));
        for (const auto &u : arrayFanLX(j.at("users"), 10000))
            s.users.push_back({strFanLX(u.at("userId")), strFanLX(u.at("nickname")),
                               strFanLX(u.at("birthDate")), strFanLX(u.at("applyDate")),
                               strFanLX(u.at("location"))});
        for (const auto &a : arrayFanLX(j.at("accounts"), 30000))
            s.accounts.push_back({strFanLX(a.at("userId")), serviceFanLX(a.at("service")),
                                  strFanLX(a.at("accountId")), boolFanLX(a.at("sharedId"))});
        for (const auto &r : arrayFanLX(j.at("subscriptions"), 30000))
            s.subscriptions.push_back(refFanLX(r));
        for (const auto &b : arrayFanLX(j.at("bindings"), 30000))
            s.bindings.push_back({refFanLX(b.at("left")), refFanLX(b.at("right"))});
        for (const auto &f : arrayFanLX(j.at("friendships"), 100000))
            s.friendships.push_back({refFanLX(f.at("left")), refFanLX(f.at("right")),
                                     strFanLX(f.at("leftRemark")), strFanLX(f.at("leftTag")),
                                     strFanLX(f.at("rightRemark")), strFanLX(f.at("rightTag"))});
        for (const auto &g : arrayFanLX(j.at("groups"), 1000))
            s.groups.push_back(groupFanLX(g));
        for (const auto &g : arrayFanLX(j.at("archivedGroups"), 1000))
            s.archivedGroups.push_back(groupFanLX(g));
        SchemaValidatorFanLX::validate(s);
        return s;
    } catch (const nlohmann::json::exception &e) {
        invalidFanLX(std::string("JSON 字段错误: ") + e.what());
    }
}
PlatformSnapshotFanLX SnapshotCodecFanLX::parse(const std::string &bytes) {
    if (bytes.size() > 64u * 1024u * 1024u)
        invalidFanLX("快照超过 64 MiB");
    // 回调在构造 DOM 期间拒绝深度过大和重复属性，避免解析后才发现分配失控。
    std::map<int, std::set<std::string>> properties;
    auto callback = [&](int depth, JsonFanLX::parse_event_t event, JsonFanLX &parsed) {
        if (depth > 32)
            invalidFanLX("JSON 嵌套过深");
        if (event == JsonFanLX::parse_event_t::object_start)
            properties[depth + 1].clear();
        if (event == JsonFanLX::parse_event_t::key &&
            !properties[depth].insert(parsed.get<std::string>()).second)
            invalidFanLX("JSON 属性重复");
        return true;
    };
    try {
        return decode(JsonFanLX::parse(bytes, callback));
    } catch (const nlohmann::json::exception &e) {
        invalidFanLX(std::string("JSON 解析失败: ") + e.what());
    }
}

void SnapshotCodecFanLX::copySessions(PlatformStateFanLX &to, const PlatformStateFanLX &from) {
    to.accounts.loggedInFanLX = from.accounts.loggedInFanLX;
}
