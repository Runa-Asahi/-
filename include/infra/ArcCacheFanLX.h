// ============================================================================
// ArcCacheFanLX.h —— ARC（Adaptive Replacement Cache，自适应替换缓存）
// ----------------------------------------------------------------------------
// 原理简述（Megiddo & Modha 提出的 ARC 算法 + 访问频率增强）：
//   缓存分为四段：
//     · L1（实际缓存）：最近进入 / 最近被访问过的"新近"条目；
//     · B1（幽灵列表）：从 L1 淘汰出去键的"幽灵记录"（只记键，不存值），
//        用于感知"被淘汰的键是否很快又被访问"这一信号；
//     · L2（实际缓存）：访问频率较高、被"晋升"过的长期热门条目；
//     · B2（幽灵列表）：从 L2 淘汰出去键的幽灵记录，与 B1 对称。
//   自适应核心：变量 pFanLX 表示 L1 段应占的目标大小（配额，范围 0..capacityFanLX）。
//   每当命中 B1/B2 的幽灵记录，说明"该段淘汰得过早"，就向对应方向调整 p，使
//   缓存容量在"新近性"（L1）与"频率"（L2）之间动态再分配，以逼近最优离线策略。
//   两种命中的含义正好相反：命中 B1 → 当初不该把 L1 的条目淘汰掉 → p+1（多给 L1 配额）；
//   命中 B2 → 当初不该把 L2 的条目淘汰掉 → p-1（把配额还给 L2）。
//   这使得 p 形成一个负反馈：哪一段被删过头，就往哪一段偏。注意 p 只是"目标值"，
//   不是硬限制，L1 可以暂时超过 p，直到下一次需要淘汰时再被修剪回来。
//   本实现的步长固定为 ±1（标准 ARC 用的是 max(|B2|/|B1|, 1) 这类自适应步长），
//   是对原算法的一个简化：收敛略慢，但更容易理解和验证。
//   频率增强：L1 中的条目被访问达到 kFanLX 次后晋升到 L2；L2 的频次用
//   LfuFrequencyFanLX 独立统计，并在平均频次过高时统一减半（normalize），
//   防止个别热点键长期霸占缓存。
// 容量约束：总条目数不超过 capacityFanLX，总字节不超过 maxBytesFanLX。
//
// 【组合关系：ARC 是"LRU 段 + LFU 控制器"拼装出来的最终缓存】
//   ARC 的四段不是四份各写一遍的链表，而是同一个可复用组件 LruListFanLX 的四个实例：
//     · l1FanLX / l2FanLX —— LruListFanLX<Key, Value>    存放真实数据（L1 新近段、L2 频率段）；
//     · b1FanLX / b2FanLX —— LruListFanLX<Key, size_t>   存放幽灵（value = 淘汰时刻的频率快照）。
//   2Q/LRU 缓存（LruCacheFanLX）使用的也是同一个组件，于是"链表 ↔ 哈希路由 ↔ 字节账目"
//   三者保持同步这套最易出错的逻辑，全项目只实现一次。
//   三个层次的职责由此清晰分离：
//     · LruListFanLX      —— 结构层：LRU 次序、O(1) 哈希定位、字节记账、队尾淘汰；
//     · LfuFrequencyFanLX —— 度量层：访问频次累计与平均频次衰减（老化）；
//     · ArcCacheFanLX     —— 策略层：p 自适应、L1→L2 晋升、B1/B2 幽灵反馈。
// ============================================================================
#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include "infra/LfuCacheFanLX.h"
#include "infra/LruListFanLX.h"

// ARC 四分区缓存：L1/L2 保存数据，B1/B2 保存被淘汰键的幽灵记录。
template <class Key, class Value, class Hash = std::hash<Key>>
class ArcCacheFanLX
{
    // 段标识：只用于在"两条数据段 / 两条幽灵段"之间做选择。
    // 注意它不再存进结点——某个条目位于哪一段，由"是哪条段回应了查找"直接决定；
    // 因此旧版本里 EntryFanLX::level 字段连同它的同步维护一起消失了。
    enum class LevelFanLX
    {
        L1, // 新近段（数据在 l1FanLX，幽灵在 b1FanLX）
        L2  // 频率段（数据在 l2FanLX，幽灵在 b2FanLX）
    };

    // 四段全部复用同一个 LRU 段组件
    using DataSegmentFanLX = LruListFanLX<Key, Value, Hash>;        // 数据段
    using GhostSegmentFanLX = LruListFanLX<Key, std::size_t, Hash>; // 幽灵段（value = 频率快照）
    using EntryFanLX = typename DataSegmentFanLX::EntryFanLX;       // 结点类型（含 key/value/bytes/freq）

    DataSegmentFanLX l1FanLX, l2FanLX;     // 两段实际缓存
    GhostSegmentFanLX b1FanLX, b2FanLX;    // 两段幽灵记录
    LfuFrequencyFanLX<Key, Hash> lfuFanLX; // L2 段频次控制器（支持整体衰减），组合复用
    // 统计与控制参数：
    //   capacityFanLX 条目数上限；maxBytesFanLX 字节数上限；pFanLX ARC 自适应目标（L1 应占条数）；
    //   kFanLX 晋升阈值（L1 访问达 k 次晋升 L2）；maxAverageFreqFanLX L2 平均频率上限（超过则减半）；
    //   totalFreqFanLX L2 频率总和（用于求平均，增量维护）。
    // 注：当前占用字节数不再单独记账——直接由 bytes() 向两条数据段要账即可，
    //     少一个必须手工同步、一旦漏改就会造成容量失控的冗余计数器。
    std::size_t capacityFanLX = 0, maxBytesFanLX = 0, pFanLX = 0, kFanLX = 2, maxAverageFreqFanLX = 32, totalFreqFanLX = 0;

    // 内部工具：按段标识取对应的幽灵段。
    GhostSegmentFanLX &ghostSegmentOfFanLX(LevelFanLX level) noexcept
    {
        return level == LevelFanLX::L1 ? b1FanLX : b2FanLX;
    }

    // 内部工具：记录一条幽灵记录（插入对应幽灵段队首），并按条数上限裁剪。
    // 幽灵只占条数、不占缓存的字节预算，故 bytes 传 0；裁剪只设条数上限。
    // 不变量：同一键在幽灵段中至多一个结点（put 会先清除同键的幽灵记录）。
    void recordGhostFanLX(const Key &key, LevelFanLX level, std::size_t freq)
    {
        GhostSegmentFanLX &list = ghostSegmentOfFanLX(level);
        list.pushFront({key, freq, 0}); // value 存淘汰时刻的频率快照
        list.evictToFit(capacityFanLX); // 超出容量从队尾丢弃（幽灵段不用字节预算）
    }

    // 内部工具：执行淘汰，直到条数与字节数都回到限额之内。
    // 【职责划分】while 条件回答"还要不要继续淘汰"，只能用硬约束（capacityFanLX / maxBytesFanLX）：
    //   这两条是本类对外承诺的容量契约，循环退出时必须保证二者同时成立；
    //   而 pFanLX 回答的是"这一轮淘汰谁"，它只是 0..capacity 的软目标（偏好），与"总量是否超限"不等价，
    //   因此绝不能拿它当循环条件——例如 p 饱和到 capacity 时 `|L1| > p` 恒为假，
    //   循环一次都不会执行，刚写入的条目就会让 size() 超出 capacity（实测 5 > 4）。
    // 决策依据是自适应目标 pFanLX（L1 应占的条数配额）：
    //   · |L1| > p  → L1 已超出配额，多出来的都是"刚来、还没证明有持久热度"的条目，先牺牲它（L1 队尾）；
    //   · |L1| ≤ p  → L1 在配额之内，改牺牲热度最低的 L2 队尾。
    // while 每轮都必须重新判断，因为淘汰一条后 |L1| 可能已回到 p 以内，下一轮就该转去修剪 L2
    // （这正是 ARC 交替修剪两段、最终把两段比例稳定在 p 附近的机制）。
    // 为什么还要判断 `|| l2FanLX.empty()`：p 会被 B1 幽灵命中一路推到 capacityFanLX 上限，
    // 这时 |L1| ≤ capacity == p，只按 |L1| > p 判断会误判成"该淘汰 L2"；若 L2 恰好为空，
    // 就没有条目可淘汰，循环只能退出，字节上限随即被突破。
    // 因此 L2 无货时必须向 L1 取（与标准 ARC 的 REPLACE 子过程一致）。
    // 【复用组件带来的简化】"该从哪一段淘汰"由 fromL1 一个布尔量表达，
    //   旧版本中"淘汰后还要从 victim->level 反查它属于哪一段"的判断彻底消失了。
    void evictFanLX()
    {
        while (bytes() > maxBytesFanLX || size() > capacityFanLX)
        {
            // 决策：是否从 L1 淘汰（L1 超出目标 p，或 L2 已空、只能动 L1）
            const bool fromL1 = l1FanLX.size() > pFanLX || l2FanLX.empty();
            DataSegmentFanLX &segment = fromL1 ? l1FanLX : l2FanLX;
            auto victim = segment.popBack(); // 摘出该段最久未用的（队尾）
            if (!victim)
                break; // 防御性兜底：按不变量此处不可达（选中的段若为空，两条循环条件都不会成立），
                       // 保留它是为了万一容量统计出错也不会陷入死循环。
            if (!fromL1) // 从 L2 淘汰还要同步清理频次账目
            {
                totalFreqFanLX -= victim->freq;
                lfuFanLX.erase(victim->key);
            }
            recordGhostFanLX(victim->key, fromL1 ? LevelFanLX::L1 : LevelFanLX::L2, victim->freq); // 转入幽灵段
        }
    }

    // 内部工具：频率归一化。当 L2 的平均频率超过 maxAverageFreqFanLX 时，
    // 将 L2 所有条目频率减半，防止"永远刷不下来的热点"霸占 L2。
    void normalizeFanLX()
    {
        if (l2FanLX.empty() || totalFreqFanLX / l2FanLX.size() <= maxAverageFreqFanLX)
            return; // 无需衰减
        totalFreqFanLX = 0;
        for (auto &item : l2FanLX) // 直接遍历 LRU 段暴露的结点范围（队首 -> 队尾）
        {
            item.freq = (item.freq + 1) / 2; // (n+1)/2：向上取半，避免频率全归零
            totalFreqFanLX += item.freq;
            lfuFanLX.set(item.key, item.freq); // 与独立的频次控制器保持同步
        }
    }

public:
    // 构造：传入条目上限与字节上限；k / maxAverageFreq 可选，k 为 0 时按 1 处理。
    // 初始化列表按"成员声明顺序"书写（lfuFanLX 在统计参数之前声明），避免 -Wreorder 警告。
    explicit ArcCacheFanLX(std::size_t maxEntries, std::size_t maxBytes, std::size_t k = 2, std::size_t maxAverageFreq = 32) : lfuFanLX(maxAverageFreq), capacityFanLX(maxEntries), maxBytesFanLX(maxBytes), kFanLX(k == 0 ? 1 : k), maxAverageFreqFanLX(maxAverageFreq)
    {
        l1FanLX.reserve(maxEntries); // 四段各自预留哈希桶位，减少扩容开销
        l2FanLX.reserve(maxEntries);
        b1FanLX.reserve(maxEntries);
        b2FanLX.reserve(maxEntries);
    }

    // 读取：命中则更新频率并刷新到队首；L1 条目频率达 k 则晋升到 L2。
    // 命中幽灵记录（B1/B2）说明该段此前淘汰过早，按 ARC 规则调整自适应参数 p。
    // 未命中返回 std::nullopt（真实数据并不在缓存中）。
    std::optional<Value> get(const Key &key)
    {
        // ---- L1 命中 ----
        if (auto node = l1FanLX.locate(key); node != l1FanLX.end())
        {
            const std::size_t freq = ++node->freq; // 访问次数 +1
            if (freq >= kFanLX)
            {
                // 新近段条目访问足够频繁：晋升到频率段 L2 队首。
                // take() 把结点内容整体搬出并摘除，pushFront() 再将其装进 L2 的新结点；
                // "为什么必须用移动而不是拷贝""为什么必须先摘路由再搬内容"这些细节，
                // 已经封装在 LruListFanLX::take 一处，策略层只表达意图。
                EntryFanLX promoted = l1FanLX.take(node);
                totalFreqFanLX += promoted.freq;           // 条目由 L1 转入 L2，从此计入 L2 频次总和
                lfuFanLX.set(promoted.key, promoted.freq); // 用同一 freq 给频次控制器播种，保持两套账一致
                l2FanLX.pushFront(std::move(promoted));    // 入 L2 队首
                normalizeFanLX();                          // 检查是否需要频率衰减
                return l2FanLX.peek(key);
            }
            l1FanLX.moveToFront(node); // 未达阈值：仅刷新为 L1 最近使用（splice 搬结点，迭代器仍有效）
            normalizeFanLX();
            return node->value;
        }
        // ---- L2 命中 ----
        if (auto node = l2FanLX.locate(key); node != l2FanLX.end())
        {
            ++node->freq;              // 条目自身频次 +1
            ++totalFreqFanLX;          // L2 频次总和 +1
            lfuFanLX.touch(key);       // 频次控制器同步 +1（内含平均频次衰减检查）
            l2FanLX.moveToFront(node); // 刷新到 L2 队首（迭代器仍有效）
            normalizeFanLX();          // 检查是否需要频率衰减
            return node->value;
        }
        // ---- 幽灵命中（B1/B2）：数据已被淘汰但近期又被访问 ----
        if (auto ghost = b1FanLX.locate(key); ghost != b1FanLX.end())
        {
            b1FanLX.take(ghost); // 幽灵记录被"消费"掉（O(1) 摘除）
            // 命中了 B1（新近段淘汰过猛）：提高 L1 的目标占比
            if (pFanLX < capacityFanLX) ++pFanLX;
            return std::nullopt;
        }
        if (auto ghost = b2FanLX.locate(key); ghost != b2FanLX.end())
        {
            b2FanLX.take(ghost);
            // 命中了 B2（频率段淘汰过猛）：降低 L1 目标，把容量让给 L2
            if (pFanLX > 0) --pFanLX;
            return std::nullopt;
        }
        return std::nullopt; // 幽灵命中不等于数据命中，仍需外部回源
    }

    // 写入：数据先进 L1 队首；若存在幽灵记录则先调整 p 并清除它。
    // 返回 false 表示容量不足（缓存为 0 或单条超限）或写入后被立即淘汰。
    bool put(Key key, Value value, std::size_t bytes = sizeof(Value))
    {
        if (capacityFanLX == 0 || bytes > maxBytesFanLX)
            return false; // 无容量或单条体积超过整体上限，直接拒绝
        erase(key); // 覆盖式写入：先删除旧数据（同时保证 L1 内键唯一）
        // 重新写入一个曾在幽灵列表里的键，说明它仍被需要：按命中 B1/B2 的同一规则微调 p，
        // 并清除旧幽灵记录，避免幽灵与真实数据共存。
        if (auto ghost = b1FanLX.locate(key); ghost != b1FanLX.end())
        {
            b1FanLX.take(ghost);
            if (pFanLX < capacityFanLX) ++pFanLX;
        }
        else if (auto ghost = b2FanLX.locate(key); ghost != b2FanLX.end())
        {
            b2FanLX.take(ghost);
            if (pFanLX > 0) --pFanLX;
        }
        const auto inserted = l1FanLX.pushFront({std::move(key), std::move(value), bytes, 1}); // 新条目入 L1 队首
        const Key insertedKey = inserted->key; // 拷贝一份用于写入后校验（evict 可能把它整条删掉）
        evictFanLX();                          // 超限则淘汰（可能连刚写入的条目一起被淘汰）
        return contains(insertedKey);          // 仍在缓存中说明写入成功
    }

    // 删除指定键。
    // 结点在哪一段，由"哪条段答得上话"直接决定，无需读取结点里的标记；
    // 于是"是否要清理 L2 频次账目"也随之变成一个自然的分支。
    void erase(const Key &key)
    {
        if (l1FanLX.erase(key))
            return; // 位于 L1：摘除即可（L1 条目不参与 L2 频次账目）
        if (auto node = l2FanLX.locate(key); node != l2FanLX.end())
        {
            totalFreqFanLX -= node->freq; // 修正 L2 频次总和
            lfuFanLX.erase(key);          // 同步清理频次控制器
            l2FanLX.take(node);
        }
    }

    // 清空缓存：四个分区与频次控制器全部清空，统计量归零。
    void clear() noexcept
    {
        l1FanLX.clear();
        l2FanLX.clear();
        b1FanLX.clear();
        b2FanLX.clear();
        lfuFanLX.clear();
        totalFreqFanLX = pFanLX = 0;
    }

    // 以下为统计查询接口。
    bool contains(const Key &key) const noexcept { return l1FanLX.contains(key) || l2FanLX.contains(key); } // 键是否在缓存中
    std::size_t size() const noexcept { return l1FanLX.size() + l2FanLX.size(); }    // 实际缓存条数（L1+L2）
    std::size_t bytes() const noexcept { return l1FanLX.bytes() + l2FanLX.bytes(); } // 当前占用字节
    std::size_t l1Size() const noexcept { return l1FanLX.size(); }                   // L1 条数
    std::size_t l2Size() const noexcept { return l2FanLX.size(); }                   // L2 条数
    std::size_t ghostSize() const noexcept { return b1FanLX.size() + b2FanLX.size(); } // 幽灵记录总数
    std::size_t targetL1() const noexcept { return pFanLX; }                         // 当前 L1 自适应目标
    std::size_t averageFrequency() const noexcept { return l2FanLX.empty() ? 0 : totalFreqFanLX / l2FanLX.size(); } // L2 平均频率（本类自算）
    // 以下两个接口让 LfuFrequencyFanLX 从"只写不读"变成有真实消费者的组件。
    std::size_t l2Frequency(const Key &key) const noexcept { return lfuFanLX.frequency(key); } // 某键在 L2 的登记频次
    std::size_t averageL2Frequency() const noexcept { return lfuFanLX.averageAccessCount(); }  // 频次控制器侧的平均频次
    // 某键被淘汰时的频率快照（0 表示没有该键的幽灵记录）。
    std::size_t ghostFrequency(const Key &key) const
    {
        if (const auto inB1 = b1FanLX.peek(key)) return *inB1;
        if (const auto inB2 = b2FanLX.peek(key)) return *inB2;
        return 0;
    }
};
