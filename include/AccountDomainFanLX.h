// ============================================================================
// AccountDomainFanLX.h —— 账户域：用户资料 / 服务账号 / 订阅 / 统一登录
// ----------------------------------------------------------------------------
// 本文件模拟即时通讯（IM）系统中的"账户"核心领域，包含三层概念：
//   1. UserProfileFanLX   用户资料：用户 ID、昵称、出生日期、注册日期、所在地；
//   2. ServiceAccountFanLX 服务账号：同一用户可在 QQ / Wechat / Weibo 等不同平台
//      分别拥有账号（sharedId 表示该账号与其他平台共用同一账号 ID，便于统一登录）；
//   3. AccountManagerFanLX 账户管理器：集中管理"用户资料 + 服务账号 + 订阅关系
//      + 绑定关系 + 登录状态"，并提供带业务校验的操作接口。
// 设计要点（面向对象）：
//   - 数据结构全部私有封装，外部只能通过公开方法访问，保证数据完整性；
//   - 内部统一用  "userId|服务名"  字符串作为存储键，减少枚举/字符串互转出错；
//   - 违反业务规则（如用户重复、账号不存在、服务未开通）时直接抛出异常，
//     让错误在第一时间暴露，而不是静默返回错误状态。
//   - 身份分层：ServiceAccountFanLX 的平台账号（accountId）才是"登录主体"——
//     现实中平台侧校验凭据后拿到的就是 QQ 号 / 微信号，再由账号反查归属用户；
//     UserProfileFanLX 的 userId 只是后台内部聚合维度，不参与登录判定。
//   - 统一登录的两条依据是"同号"（sharedId 复用同一账号 ID）与"显式绑定"
//     （bindAccounts，前端经会话 bindTo 调用），而不是"属于同一个人"——后者会把无关平台一并登录。
//   - 本域约定"一个用户在一个平台只持有一个账号"（见 addAccount），
//     故 "userId|服务名" 恰是该平台账号的无损别名，可直接用作存储键。
//   - 接口分层（rev2 确立，对应工程计划 §4.1 的三层身份）：
//       · 前端 API = 登录会话 AccountSessionFanLX。凡"由某个已登录用户发起"的操作
//         一律经由会话，签名中不出现 userId；对方一律以平台账号 (service, accountId) 指定。
//       · 后端 API = AccountManagerFanLX 上与 userId 相关的配给/统计/持久化操作
//         （注册用户、建号、开通、按用户查询）。userId 是内部不可变主键。
//   - 业务错误分类（对应工程计划"接口错误能区分未开通、未确认、已过期"）。
// ============================================================================
#pragma once
// 兼容入口，业务类型按课程要求独立存放。
#include "account/AccountManagerFanLX.h"
#include "account/AccountSessionFanLX.h"
#include "account/AccountRefFanLX.h"
#include "account/UserProfileFanLX.h"
#include "account/ServiceAccountFanLX.h"
#include "account/ServiceTypeFanLX.h"
#include "account/BindingSnapshotFanLX.h"
