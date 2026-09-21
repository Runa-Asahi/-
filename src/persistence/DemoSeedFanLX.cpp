#include "friend/IdentityResolverFanLX.h"
#include "persistence/DemoSeedFanLX.h"
#include "persistence/SnapshotCodecFanLX.h"

PlatformSnapshotFanLX demoSeedFanLX() {
    PlatformStateFanLX state;
    for (int n = 1; n <= 12; ++n) {
        const auto id = "U" + std::to_string(n);
        const auto number = "202600" + std::to_string(n);
        state.accounts.addUser(
            {id, n == 1 ? "樊陆旭" : "同学" + std::to_string(n), "2005-01-01", "2026-09-01", "吉林"});
        for (auto service : {ServiceTypeFanLX::QQ, ServiceTypeFanLX::Weibo, ServiceTypeFanLX::Wechat}) {
            state.accounts.addAccount({id, service,
                                       service == ServiceTypeFanLX::Wechat ? "wx_" + number : number,
                                       service != ServiceTypeFanLX::Wechat});
            state.accounts.subscribe(id, service);
        }
        state.accounts.bindAccounts(ServiceTypeFanLX::Wechat, "wx_" + number, ServiceTypeFanLX::QQ, number);
    }
    for (auto mode : {GroupModeFanLX::QQ, GroupModeFanLX::Wechat})
        for (int n = 1001; n <= 1006; ++n) {
            auto &group = state.groups.create({mode, std::to_string(n)});
            group.addMember("U1", GroupRoleFanLX::Owner);
            group.addMember("U2");
            group.addMember("U3");
        }
    const GroupKeyFanLX qq{GroupModeFanLX::QQ, "1001"};
    state.groups.assignAdmin(qq, "U1", "U2");
    state.groups.createDiscussion(qq, "D1", "U1");
    state.groups.joinDiscussion(qq, "D1", "U1", "U1");
    state.groups.joinDiscussion(qq, "D1", "U1", "U3");
    state.groups.applyToJoin(qq, "U4");
    state.groups.inviteMember({GroupModeFanLX::Wechat, "1001"}, "U1", "U4");
    IdentityResolverFanLX resolver(state.accounts);
    auto left = resolver.accountKey(ServiceTypeFanLX::QQ, "2026001");
    auto right = resolver.accountKey(ServiceTypeFanLX::QQ, "2026002");
    state.friends.add(left, right);
    state.friends.update(left, right, "课程搭档", "同学");
    state.friends.update(right, left, "樊同学", "好友");
    return SnapshotCodecFanLX::capture(state);
}
