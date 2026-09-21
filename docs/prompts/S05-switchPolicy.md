## 任务：实现 S05 群模式无损切换（switchPolicy）

### 项目背景

C++ 面向对象课程设计，模拟即时通讯系统。
工具链：CMake 3.20+ / MinGW（D:/mingw64）/ C++17（严禁使用 C++20 语法，如 operator== = default）
项目根目录：C:\Users\25686\Desktop\OOP

请先完整阅读 docs/progress.md 中「S05 设计与约束」一节，本提示词是该设计的执行细则，两者冲突时以 docs/progress.md 为准。同时对齐工程文档（deliverables/樊陆旭_面向对象课设工程文档.docx）中「二、设计方案」与「三、详细设计」关于群模式无损切换与权限矩阵的说明。

---

### 核心设计原则（务必理解后再动手）

1. `GroupKeyFanLX.mode` 是群的"创建时所属命名空间"，创建后永久冻结，不参与切换。
2. 群"当前生效治理模式"是独立、可变的属性，来自 `policyFanLX->mode()`。
3. `switchPolicy` 只做一件事：交换 `policyFanLX` 指针。绝不触碰 `membersFanLX`、`discussionsFanLX`、`applicationsFanLX`、`invitationsFanLX`。—— 这是"无损"的根本来源：数据从未被移动过，只是权限计算时参照的策略变了。
4. 新策略必须由**调用方**构造完成后再传入 `switchPolicy`；若构造抛异常，`switchPolicy` 从未被调用，原策略天然保持不变——不需要任何显式回滚代码。

---

### 需要修改的文件（群策略族 + 群域类型 + 对应测试文件）

#### 1. `include/group/GroupPolicyFanLX.h`

在 `GroupPolicyFanLX` 抽象基类中新增两个纯虚方法：

```cpp
virtual GroupModeFanLX mode() const = 0;
virtual bool canAssignAdmin() const = 0;
```

在 `QQGroupPolicyFanLX` 中实现：`mode()` 返回 `GroupModeFanLX::QQ`，`canAssignAdmin()` 返回 `true`。

在 `WechatGroupPolicyFanLX` 中实现：`mode()` 返回 `GroupModeFanLX::Wechat`，`canAssignAdmin()` 返回 `false`。

不修改现有的 `canCreateDiscussion` / `name`。

#### 2. `include/GroupDomainFanLX.h`（聚合入口；群域各类型已按类拆到 `include/group/`）

**2.1 成员存储改造（为支持"加入时间"无损对比）**

新增结构体：

```cpp
struct GroupMembershipFanLX { GroupRoleFanLX role; std::size_t joinOrder; };
```

将 `GroupFanLX::membersFanLX` 的值类型从 `GroupRoleFanLX` 改为 `GroupMembershipFanLX`；新增私有成员 `std::size_t nextJoinOrderFanLX = 0;`。

同步修改以下方法内部实现（**公开签名不变**，保持与现有 T06–T09 测试完全兼容）：

- `addMember`：先用 `count` 判断是否已存在，不存在才 `emplace(id, {role, nextJoinOrderFanLX++})`，避免失败插入消耗序号。
- `setRole`：只改 `.role` 字段，不改 `joinOrder`。
- `roleOf`：返回 `.role`。
- `members()`：逻辑不变，只是取 `.first`（key），无需改动取值方式。
- `transferOwnership`：遍历时改 `.second.role`（不是 `.second`），双方 `joinOrder` 保持不变——这是"无损"的关键：转让群主不会重置任何人的加入时间。
- `leave`：逻辑不变。

**2.2 权限矩阵改为策略感知**

修改 `removeMember`，在判断 actor 是否为"生效管理员"时新增策略检查：

```cpp
const bool actorIsEffectiveAdmin =
    actorRole == GroupRoleFanLX::Admin && policyFanLX->canAssignAdmin();
```

只有 `actorIsEffectiveAdmin == true` 才继续走"Admin 不能踢同级 Admin"的判断；否则视为无权（与 Member 同等对待，返回 false）。其余判断顺序（不能踢 Owner、不能踢自己、Owner 可踢任何非 Owner）保持不变。

**2.3 新增 currentMode 与 switchPolicy（GroupFanLX）**

```cpp
GroupModeFanLX currentMode() const noexcept { return policyFanLX->mode(); }

void switchPolicy(std::unique_ptr<GroupPolicyFanLX> newPolicy) {
    if (!newPolicy) throw std::invalid_argument("目标策略不可为空");
    policyFanLX = std::move(newPolicy);
}
```

**2.4 新增快照对比接口（GroupFanLX）**

```cpp
struct GroupMemberSnapshotFanLX {
    std::string id;
    GroupRoleFanLX role;
    std::size_t joinOrder;
    bool operator==(const GroupMemberSnapshotFanLX& o) const noexcept {
        return id == o.id && role == o.role && joinOrder == o.joinOrder;
    }
};

std::vector<GroupMemberSnapshotFanLX> snapshot() const {
    // 遍历 membersFanLX，构造 {id, role, joinOrder}，按 id 字典序排序后返回
}
```

禁止使用 `= default`（C++20 语法，项目为 C++17）。

**2.5 requireMode 改为按群当前模式判断（GroupServiceFanLX）**

将：

```cpp
static void requireMode(const GroupKeyFanLX& key, GroupModeFanLX mode)
```

改为：

```cpp
static void requireMode(const GroupFanLX& group, GroupModeFanLX mode) {
    if (group.currentMode() != mode) throw std::runtime_error("当前群模式不支持该加入方式");
}
```

并同步修改全部 6 个调用点（`applyToJoin`、`approveApplication`、`rejectApplication`、`inviteMember`、`confirmInvitation`、`rejectInvitation`）：先 `get(key)` 拿到 group 引用，再传 group 给 `requireMode`（大部分调用点已经先 `get(key)`，只需调整参数传递顺序）。

**2.6 新增 Service 层方法**

```cpp
// 切换（通用 + 两个便捷封装）
void switchPolicy(const GroupKeyFanLX& key, const std::string& actor,
                   std::unique_ptr<GroupPolicyFanLX> newPolicy) {
    auto& group = get(key);
    requireOwner(group, actor);
    group.switchPolicy(std::move(newPolicy));
}
void switchToQQ(const GroupKeyFanLX& key, const std::string& actor) {
    switchPolicy(key, actor, std::unique_ptr<GroupPolicyFanLX>(new QQGroupPolicyFanLX()));
}
void switchToWechat(const GroupKeyFanLX& key, const std::string& actor) {
    switchPolicy(key, actor, std::unique_ptr<GroupPolicyFanLX>(new WechatGroupPolicyFanLX()));
}

// 管理员任免（Owner 专属；assignAdmin 受 canAssignAdmin() 门控，revokeAdmin 始终允许）
void assignAdmin(const GroupKeyFanLX& key, const std::string& actor, const std::string& target) {
    auto& group = get(key);
    requireOwner(group, actor);
    if (!group.policy().canAssignAdmin()) throw std::runtime_error("当前模式不支持设置管理员");
    group.setRole(target, GroupRoleFanLX::Admin);
}
void revokeAdmin(const GroupKeyFanLX& key, const std::string& actor, const std::string& target) {
    auto& group = get(key);
    requireOwner(group, actor);
    if (group.roleOf(target) != GroupRoleFanLX::Admin) throw std::runtime_error("目标不是管理员");
    group.setRole(target, GroupRoleFanLX::Member);
}

// 子群扩员（受 canCreateDiscussion() 门控，与新建子群共用同一开关）
bool joinDiscussion(const GroupKeyFanLX& parentKey, const std::string& discId,
                     const std::string& actor, const std::string& target) {
    auto& group = get(parentKey);
    requireManager(group, actor);
    if (!group.canCreateDiscussion()) throw std::runtime_error("当前策略禁止子群扩员");
    return getDiscussion(parentKey, discId).addMember(group, target);
}
```

---

### 测试：在 `tests/group_tests.cpp` 追加 T10（保留 T06–T09 原样不动）

新增一个 `// T10：...` 注释分节，覆盖以下场景（用 `<cassert>`，沿用文件已有的 `expectExceptionFanLX` 辅助模板）：

**T10a 权限矩阵随切换变化**

- 建 QQ 群，Owner 设置一名 Admin；QQ 模式下 Admin 能踢 Member、能建子群
- `switchToWechat` 后，同一 Admin 尝试踢 Member 失败、尝试建子群抛异常；Owner 仍可踢人、仍可管理

**T10b 双向切换无损**

- 记录 `snapshot()`
- 连续 `switchToWechat` → `switchToQQ`，中间不做任何其他业务修改
- 断言前后 `snapshot()` 相等（`vector` 直接 `==` 比较）
- 再验证反向：`switchToQQ` → `switchToWechat` 同样无损（若群已在 QQ，先切到 Wechat 建立基线）

**T10c 休眠与恢复语义**

- QQ 模式下 `assignAdmin` 一人为 Admin
- 切到 Wechat：断言 `roleOf` 仍返回 `Admin`（休眠数据保留），但该人 `removeMember` 失去踢人权
- 切回 QQ：断言该人权限自动恢复（能再次成功踢人），无需任何显式"恢复"调用
- 另找一人在 Wechat 模式下被 Owner `revokeAdmin` 降级为 Member
- 切回 QQ：断言其角色仍是 Member（"已被群主撤销的人不恢复权限"）

**T10d 子群只读冻结**

- QQ 模式下建子群并加入一人
- 切到 Wechat：断言 `joinDiscussion`（扩员）抛异常，`createDiscussion`（新建）抛异常
- 父群成员此时主动退群：断言其子群席位仍被同步清除（不受冻结影响）

**T10e 失败路径不产生半切换**

- 非 Owner 调用 `switchPolicy` → 抛异常；切换前后 `policy().name()` 与 `snapshot()` 均不变
- 传入 `nullptr` 策略 → 抛异常；同上验证不变
- 在测试文件内定义一个仅测试用的抛异常策略子类（构造函数体 `throw`），验证构造失败时 `switchPolicy` 从未被调用、原策略对象保持原样

---

### 硬性约束

1. 不使用任何 C++20 语法（项目 `CMAKE_CXX_STANDARD 17`），包括 `operator== = default`
2. 不修改 `GroupApplicationFanLX`、`GroupInvitationFanLX`、`DiscussionGroupFanLX` 的现有字段/方法签名
3. 不修改 `applyToJoin`、`approveApplication`、`rejectApplication`、`inviteMember`、`confirmInvitation`、`rejectInvitation`、`leave`、`dissolve`、`transferOwnership`、`createDiscussion`、`getDiscussion` 的**公开签名**（内部实现按 2.5 节调整 requireMode 调用即可）
4. 不修改 `tests/cache_tests.cpp`、`AccountDomainFanLX.h`、`FriendDomainFanLX.h`
5. T06–T09 必须保持全部通过，不允许因本次改动回归
6. 不实现持久化（重启保留新模式、写盘失败回滚均为 S06 范围，本轮不涉及）
7. 不扩展 Weibo 群模式或三方切换
8. `src/main.cpp` 的 `--demo` 本轮**不要求**新增切换演示（可选，非必需）

### 验收标准

完成后运行：

```bash
cmake --build build -- -j4
ctest --test-dir build --output-on-failure
```

必须：编译无警告，CTest **3/3** 全通过（T10 并入 group_behavior 测试可执行文件，不新增 CMakeLists 条目），clangd 零诊断。
