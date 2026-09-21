// ============================================================================
// LruCacheFanLX.h —— 2Q / LRU-K 缓存（近期 + 长期两级 LRU 队列）
// ----------------------------------------------------------------------------
// 背景：朴素 LRU 只保留"最近使用"，一旦发生顺序扫描，冷数据会污染缓存。
//       本实现采用 2Q（Two Queues）思路：把缓存分成两级——
//         · Buffer（短期缓冲队列）：新数据先进这里，只出现一次的"过客"数据
//           会在这里被淘汰，不污染主缓存；
//         · Main（长期主队列）   ：数据被访问达到 K 次后晋升到此，代表"真正热
//           门、值得长期保留"的条目，按 LRU 顺序淘汰。
//       因此它同时具备 LRU-K 的"按访问次数晋升"特性：kFanLX 即晋升阈值。
// 组合方式（与 ARC 深度融合的落点）：
//   两条队列都不再手写链表，而是各复用一条 LRU 段组件 LruListFanLX：
//         Buffer = 一条 LruListFanLX，Main = 另一条 LruListFanLX。
//   "哈希定位 + LRU 次序 + 字节记账 + 队尾淘汰"全部由组件提供，
//   本类只保留策略本身：容量按 1:3 切分、Buffer 中访问达 K 次即晋升 Main。
//   ARC 缓存（ArcCacheFanLX）使用的是同一个组件，两者共享同一套底层实现，
//   因此这套"链表 ↔ 哈希路由 ↔ 字节账目"三者同步的逻辑全项目只写一遍。
// ============================================================================
#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include "infra/LruListFanLX.h"

// 2Q/LRU-K 缓存：Buffer（短期）与 Main（长期）两条 LRU 段。
template <class Key, class Value, class Hash = std::hash<Key>>
class LruCacheFanLX
{
    using SegmentFanLX = LruListFanLX<Key, Value, Hash>;  // 复用的 LRU 段组件
    using EntryFanLX = typename SegmentFanLX::EntryFanLX; // 结点类型（key/value/bytes/freq）

    SegmentFanLX bufferFanLX, mainFanLX; // Buffer / Main 两条 LRU 段（队首=最新）
    // 两段各自的条目数与字节数上限（构造时按 1:3 分给 Buffer 与 Main）
    std::size_t bufferMaxEntriesFanLX = 0, mainMaxEntriesFanLX = 0, bufferMaxBytesFanLX = 0, mainMaxBytesFanLX = 0;
    std::size_t kFanLX = 2; // 晋升阈值：Buffer 中访问达 K 次即晋升 Main

    // 任意一段中是否存在该键（写入后校验用）。
    bool containsInAnyFanLX(const Key &key) const noexcept
    {
        return bufferFanLX.contains(key) || mainFanLX.contains(key);
    }

public:
    // 构造：总容量按比例拆分——约 1/4 给 Buffer（吸收冷数据），其余给 Main。
    explicit LruCacheFanLX(std::size_t maxEntries, std::size_t maxBytes, std::size_t k = 2)
        : bufferMaxEntriesFanLX(maxEntries / 4), mainMaxEntriesFanLX(maxEntries - maxEntries / 4),
          bufferMaxBytesFanLX(maxBytes / 4), mainMaxBytesFanLX(maxBytes - maxBytes / 4),
          kFanLX(k == 0 ? 1 : k)
    {
        bufferFanLX.reserve(maxEntries); // 两段各自预留哈希桶位，减少扩容开销
        mainFanLX.reserve(maxEntries);
    }

    // 读取：命中则刷新为所在队列的最近使用；Buffer 中条目访问达 K 次则晋升 Main。
    // 返回 std::optional 包装的值，未命中返回 std::nullopt。
    std::optional<Value> get(const Key &key)
    {
        // ---- Buffer 命中 ----
        if (auto node = bufferFanLX.locate(key); node != bufferFanLX.end())
        {
            ++node->freq;                  // 访问次数 +1
            bufferFanLX.moveToFront(node); // 先刷新为 Buffer 最近使用（splice 搬结点，迭代器仍有效）
            if (node->freq >= kFanLX)
            {
                // 达到 K 次访问：整条搬入 Main 队首。
                // take() 把内容整体搬出并摘除，pushFront() 再装进 Main 的新结点；
                // 移动语义的要点与"先摘路由再搬内容"的顺序约束都已封装在组件里。
                EntryFanLX promoted = bufferFanLX.take(node);
                mainFanLX.pushFront(std::move(promoted));
                mainFanLX.evictToFit(mainMaxEntriesFanLX, mainMaxBytesFanLX); // Main 可能超限，立即裁剪
                return mainFanLX.peek(key); // 晋升后若立即被裁剪掉，则视为未命中
            }
            return node->value; // 未达阈值：仍在 Buffer 中，直接返回值
        }
        // ---- Main 命中 ----
        if (auto node = mainFanLX.locate(key); node != mainFanLX.end())
        {
            ++node->freq;                  // 命中一律计数（Main 的频次不再用于晋升，仅作记录）
            mainFanLX.moveToFront(node);   // 刷新为 Main 最近使用
            return node->value;
        }
        return std::nullopt; // 两段都未命中
    }

    // 写入：新条目一律先进入 Buffer 队首；若放入后被淘汰则返回 false（容量不足）。
    bool put(Key key, Value value, std::size_t bytes = sizeof(Value))
    {
        if (bufferMaxEntriesFanLX == 0 || bytes > bufferMaxBytesFanLX)
            return false; // 容量为 0 或单条超过 Buffer 上限：无法放入
        erase(key); // 先移除旧值（若已存在），保证 Buffer 内键唯一
        const auto inserted = bufferFanLX.pushFront({std::move(key), std::move(value), bytes, 1});
        const Key insertedKey = inserted->key; // 拷贝一份：下面裁剪时该结点可能被删掉
        bufferFanLX.evictToFit(bufferMaxEntriesFanLX, bufferMaxBytesFanLX); // 放入后可能超限，触发淘汰
        return containsInAnyFanLX(insertedKey); // 仍存在说明未被立即淘汰
    }

    // 删除指定键（无论位于 Buffer 还是 Main）。
    void erase(const Key &key)
    {
        if (!bufferFanLX.erase(key))
            mainFanLX.erase(key);
    }

    // 清空缓存：两条队列全部清空。
    void clear() noexcept
    {
        bufferFanLX.clear();
        mainFanLX.clear();
    }

    // 以下为统计查询接口：当前总条数 / 总字节 / Buffer 条数 / Main 条数。
    std::size_t size() const noexcept { return bufferFanLX.size() + mainFanLX.size(); }
    std::size_t bytes() const noexcept { return bufferFanLX.bytes() + mainFanLX.bytes(); }
    std::size_t bufferSize() const noexcept { return bufferFanLX.size(); }
    std::size_t mainSize() const noexcept { return mainFanLX.size(); }
};
