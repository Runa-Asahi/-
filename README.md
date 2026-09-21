# 模拟即时通讯系统

C++17 面向对象课程设计 · 2026-2027 学年第一学期 · 552502 班

S06 已接入文件持久化、候选事务、稳定群外壳、恢复确认与启动/保存关闭。账户/好友/群的兼容聚合头文件仍可 include，自编领域类已拆为独立文件。S08 TCP、S09 完整数字菜单及 S07 业务缓存/性能基准仍待后续阶段。

## 构建与测试

Windows、CMake 3.20+、MinGW（本机 GCC 15.1.0，D:/mingw64），不依赖 Visual Studio。固定 JSON 头文件及 MIT 许可证随源码提供；CMake 配置/构建无需联网。

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -- -j4
ctest --test-dir build --output-on-failure
.\build\fanlx_app.exe --demo
```

四项 CTest：smoke、cache_behavior、group_behavior、persistence_behavior。Debug/Release 均启用关键断言；持久化库、入口及新增测试以 -Wall -Wextra -Wpedantic -Werror 编译。

## 真实数据目录

```powershell
# 显式创建新演示库；已有主文件/备份时拒绝初始化覆盖
.\build\fanlx_app.exe --data-dir data-classroom --init-demo
# 第二次启动：读取已有快照，检查保存并关闭
.\build\fanlx_app.exe --data-dir data-classroom
# 主文件损坏且备份有效时：显式确认恢复（先保留损坏证据）
.\build\fanlx_app.exe --data-dir data-classroom --recover
```

无参数使用当前目录的 data。空库/损坏库不会被自动用空状态覆盖。启动失败或需确认时不进入可写状态；当前入口为 S06 的启动/退出流程，完整交互菜单归 S09。

--demo 使用系统临时目录下的唯一 fanlx-demo-* 子目录，输出证据目录；展示真实写入、无损切换、关闭重开、全字段比较与登录态清空。测试只清理自己创建的唯一临时目录，失败时保留并输出路径。没有全局数据重置命令。

同一数据目录只允许一个进程持有 .lock 独占句柄；不要删除锁文件抢占。异常进程终止后 Windows 释放句柄，可直接重启。

## 目录结构

源码按四层架构分目录，每个类一个文件（课程要求：类的设计均以独立文件存在）。

```
include/
├── AccountDomainFanLX.h       # 账户域入口（聚合全部账户类型）
├── FriendDomainFanLX.h        # 好友域入口（聚合全部好友类型）
├── GroupDomainFanLX.h         # 群域入口（聚合全部群类型）
├── account/                   # 账户域：资料 / 平台账号 / 开通 / 会话 / 绑定（10 文件）
├── friend/                    # 好友域：目录 / 身份解析 / 门面 / 快照（5 文件）
├── group/                     # 群域：成员 / 角色 / 权限 / 工作流 / 子群 / 策略族（17 文件）
├── infra/                     # 基础设施：LRU 段底座 / 2Q / ARC / 频次控制器 / 内存池 / 时钟（6 文件）
└── persistence/               # 持久化：仓储 / 编解码 / 校验 / 原子写 / 事务 / 恢复（15 文件）
src/
├── main.cpp                   # 演示与启动入口
└── persistence/               # 持久化层实现（9 个 .cpp）
tests/
├── cache_tests.cpp            # 缓存与好友域断言测试
├── group_tests.cpp            # 群域权限矩阵与无损切换
└── persistence_tests.cpp      # T11/T12 与故障注入
```

约定：`#include` 路径一律以 `include/` 为根（如 `#include "group/GroupFanLX.h"`）；三个 `*DomainFanLX.h` 是该域全部类型的聚合入口，新读者可从它们入手。

## 文档与代码入口

- docs/progress.md：S06 实施与后续路线。
- docs/data-format.md：schemaVersion=1、文件协议与恢复边界。
- docs/dependencies.md：固定版本、许可、SHA-256。
- docs/persistence-validation.md：真实测试记录与未测故障边界。
- include/persistence/PersistenceApplicationFanLX.h：应用写门面。
- include/persistence/RepositoryFanLX.h：可替换仓储接口。
- src/persistence/TransactionCoordinatorFanLX.cpp：候选提交与局部版本。
- src/persistence/PreparedCommitFanLX.cpp：预分配无异常发布。
- tests/persistence_tests.cpp：T11/T12、子进程重启/崩溃与故障注入。
- tests/fixtures/persistence/valid-state-v1.json：程序实际生成的合法快照。

课程原始任务书保持原样。deliverables 中已有工程报告的功能状态/文件数量为此前版本，S11 需依据当前代码更新最终报告，不能用旧报告中的规模数字作为本轮验收证据。
