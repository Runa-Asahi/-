#include "infra/ArcCacheFanLX.h"
#include "persistence/DemoSeedFanLX.h"
#include "persistence/PersistenceApplicationFanLX.h"
#include <chrono>
#include <iostream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
void demoFanLX() {
    // 每次演示使用新目录，保留实测快照用于复查，不触碰默认 data。
    const auto directory =
        std::filesystem::temp_directory_path() /
        ("fanlx-demo-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
    const AccountRefFanLX owner{ServiceTypeFanLX::QQ, "2026001"};
    const AccountRefFanLX applicant{ServiceTypeFanLX::QQ, "2026004"};
    const GroupKeyFanLX key{GroupModeFanLX::QQ, "1001"};
    nlohmann::json saved;
    {
        PersistenceApplicationFanLX app(directory);
        app.initialize();
        app.importInitial(demoSeedFanLX());
        app.login(owner, true);
        app.login(applicant);
        app.approveApplication(owner, key, applicant);
        app.updateRemark(owner, {ServiceTypeFanLX::QQ, "2026002"}, "已保存的课程搭档", "同学");
        const auto before = app.state().groups.get(key).snapshot();
        const auto *address = &app.state().groups.get(key);
        app.switchPolicy(owner, key, GroupModeFanLX::Wechat);
        if (before != app.state().groups.get(key).snapshot() || address != &app.state().groups.get(key))
            throw std::runtime_error("切换无损检查失败");
        std::cout << "Group switch: members preserved, stable address=true\n";
        saved = SnapshotCodecFanLX::encode(app.snapshot());
        app.saveAndClose();
    }
    {
        PersistenceApplicationFanLX restarted(directory);
        if (saved != SnapshotCodecFanLX::encode(restarted.snapshot()))
            throw std::runtime_error("重启快照不一致");
        std::cout << "Restart: generation=" << restarted.state().generation
                  << " mode=" << restarted.state().groups.get(key).policy().name()
                  << " logged-in=" << std::boolalpha
                  << restarted.state().accounts.isAccountLoggedIn(owner.service, owner.accountId) << '\n';
        restarted.saveAndClose();
    }
    ArcCacheFanLX<std::string, int> cache(8, 4096);
    cache.put("demo", 42);
    std::cout << "ARC query=" << *cache.get("demo") << "\nEvidence directory: " << directory.u8string()
              << '\n';
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    SetConsoleOutputCP(CP_UTF8);
    try {
        std::filesystem::path data = L"data";
        bool initialize = false, recover = false;
        for (int i = 1; i < argc; ++i) {
            const std::wstring arg = argv[i];
            if (arg == L"--demo") {
                demoFanLX();
                return 0;
            }
            if (arg == L"--data-dir" && i + 1 < argc)
                data = argv[++i];
            else if (arg == L"--init-demo")
                initialize = true;
            else if (arg == L"--recover")
                recover = true;
            else
                throw std::invalid_argument("用法: --demo 或 [--data-dir 路径] [--init-demo | --recover]");
        }
        if (initialize && recover)
            throw std::invalid_argument("初始化与恢复不能同时执行");
        PersistenceApplicationFanLX app(data);
        std::cout << app.loadResult().message << '\n';
        if (recover)
            app.confirmRecovery();
        else if (initialize) {
            app.initialize();
            app.importInitial(demoSeedFanLX());
        } else if (app.loadResult().status != LoadStatusFanLX::Loaded) {
            std::cout << "未进入可写状态。空目录使用 --init-demo；有效备份恢复使用 --recover。\n";
            return 2;
        }
        std::cout << "Loaded users=" << app.snapshot().users.size()
                  << " generation=" << app.state().generation << '\n';
        // S09 将在这里接入完整数字菜单；S06 已实际接入启动与显式保存关闭。
        app.saveAndClose();
        std::cout << "保存检查完成，已关闭。\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "操作失败: " << e.what() << '\n';
        return 1;
    }
}
