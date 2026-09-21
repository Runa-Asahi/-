// ============================================================================
// LruListFanLX.h —— 可复用的「LRU 段」基础组件
// ----------------------------------------------------------------------------
// 定位：本组件不实现任何完整的缓存策略，只提供各种缓存策略都必需的那套底层机制——
//         · 双向链表 std::list        —— 维护 LRU 次序（队首最新、队尾最久未用）；
//         · 哈希路由表 unordered_map  —— 由键 O(1) 定位到链表结点；
//         · 字节账目 usedBytesFanLX   —— 汇总段内结点占用，供容量控制使用。
//       完整策略由若干条本段拼装而成：
//         · LruCacheFanLX（2Q）  = Buffer 段 + Main 段；
//         · ArcCacheFanLX        = L1 段 + L2 段 + B1 段 + B2 段。
//       于是"链表 ↔ 哈希路由 ↔ 字节账目"三者保持同步这套最易出错的逻辑全项目只写一次，
//       这正是把 2Q 与 ARC 深度融合的着力点。
//
// 关键约定（不变量）：
//   ① 同一键在段内至多存在一个结点；pushFront 之前由调用方保证先清除同键（缓存侧先 erase）；
//   ② 路由表与链表任何时刻都保持同步，任何新增 / 摘除都同时更新二者；
//   ③ std::list 的迭代器在"其余结点"增删时保持稳定，因此路由表可以长期缓存迭代器；
//      但结点本身一旦被摘除，其迭代器立即失效，必须重新定位后再使用。
// ============================================================================
#pragma once
#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

// 一条有界 LRU 段：哈希路由 + 双向链表 + 字节账目。
template <class Key, class Value, class Hash = std::hash<Key>>
class LruListFanLX
{
public:
    // 段内结点。用聚合初始化构造，例如 pushFront({key, value, bytes, 1})。
    // 幽灵段复用本组件时：value 存放"淘汰时刻的频率快照"，bytes 恒为 0（不参与字节预算），
    // freq 字段不使用。
    struct EntryFanLX
    {
        Key key;               // 键（同时是路由表的键）
        Value value;           // 值
        std::size_t bytes = 0; // 估算占用字节数（幽灵段恒为 0）
        std::size_t freq = 1;  // 段内累计访问次数（幽灵段不使用）
    };

    using ListFanLX = std::list<EntryFanLX>;
    using IteratorFanLX = typename ListFanLX::iterator;
    using ConstIteratorFanLX = typename ListFanLX::const_iterator;

private:
    ListFanLX listFanLX;                                     // LRU 次序：队首=最新，队尾=最久未用
    std::unordered_map<Key, IteratorFanLX, Hash> routeFanLX; // 键 -> 结点，O(1) 定位
    std::size_t usedBytesFanLX = 0;                          // 段内结点字节总和（增量维护）

    // 内部工具：把结点从段中摘除，并同步路由表与字节账目（不搬运结点内容）。
    // 供"只关心淘汰、不关心内容"的场景使用（如按容量裁剪幽灵段）。
    // 注意顺序：必须先摘路由项，再销毁结点——结点一旦销毁，就再也无法由它取到键。
    void unlinkFanLX(IteratorFanLX node)
    {
        usedBytesFanLX -= node->bytes;
        routeFanLX.erase(node->key);
        listFanLX.erase(node);
    }

public:
    // 预留路由表桶位，减少扩容开销。
    void reserve(std::size_t expected) { routeFanLX.reserve(expected); }

    bool empty() const noexcept { return listFanLX.empty(); }
    std::size_t size() const noexcept { return listFanLX.size(); }
    std::size_t bytes() const noexcept { return usedBytesFanLX; }

    // 遍历段内结点（链表顺序：队首 -> 队尾，即 最新 -> 最久未用）。
    IteratorFanLX begin() noexcept { return listFanLX.begin(); }
    IteratorFanLX end() noexcept { return listFanLX.end(); }
    ConstIteratorFanLX begin() const noexcept { return listFanLX.begin(); }
    ConstIteratorFanLX end() const noexcept { return listFanLX.end(); }

    // 键是否在段内（O(1)）。
    bool contains(const Key &key) const noexcept { return routeFanLX.find(key) != routeFanLX.end(); }

    // 由键定位结点；未命中返回 end()。返回的迭代器可直接修改结点内容。
    IteratorFanLX locate(const Key &key) noexcept
    {
        const auto it = routeFanLX.find(key);
        return it == routeFanLX.end() ? listFanLX.end() : it->second;
    }

    // 只读取值，不改变 LRU 次序；未命中返回 std::nullopt。
    std::optional<Value> peek(const Key &key) const
    {
        const auto it = routeFanLX.find(key);
        return it == routeFanLX.end() ? std::nullopt : std::optional<Value>(it->second->value);
    }

    // 新结点入队首，登记路由并累加字节账目，返回指向新结点的迭代器。
    // 调用方负责保证键不重复（否则旧结点会变成无人引用的孤儿）。
    IteratorFanLX pushFront(EntryFanLX entry)
    {
        listFanLX.push_front(std::move(entry));
        const auto node = listFanLX.begin();
        usedBytesFanLX += node->bytes;
        routeFanLX[node->key] = node;
        return node;
    }

    // 把已有结点刷新为"最近使用"（splice 在同一链表内搬移结点）。
    // std::list::splice 只改指针、不移动元素、不分配、不析构，且 node 在调用后依然有效
    // （只是位置变成了队首），因此调用方可以继续使用同一个迭代器。
    void moveToFront(IteratorFanLX node) noexcept { listFanLX.splice(listFanLX.begin(), listFanLX, node); }

    // 取出结点：把内容"搬"到一个新对象并返回，同时将结点从段中摘除。
    // 【关于 std::move】std::move 本身不搬运数据，只是把表达式转成右值引用，让编译器选中
    // 移动构造函数；真正"偷走" std::string 等堆对象内部指针的是 EntryFanLX 的隐式移动构造。
    // 标量成员（bytes/freq）本来就是按位复制，移动与否没有区别。
    // 【为什么顺序不能变】必须"先摘路由、再搬内容"，因为键一旦被 move 走就变成空值，
    // 之后再拿 node->key 去删除路由项，删的就是空键——真正的路由项会残留在表里，
    // 变成指向已销毁结点的悬垂迭代器。搬走内容后即可安全销毁这个空壳结点。
    EntryFanLX take(IteratorFanLX node)
    {
        usedBytesFanLX -= node->bytes;
        routeFanLX.erase(node->key);            // ① 趁 key 还在，先摘掉路由项
        EntryFanLX taken = std::move(*node);    // ② 把内容搬到返回值（堆缓冲区所有权转移）
        listFanLX.erase(node);                  // ③ 销毁被掏空的结点，node 就此失效
        return taken;
    }

    // 淘汰队尾（最久未用）结点并返回其内容；段为空时返回 std::nullopt。
    std::optional<EntryFanLX> popBack()
    {
        if (listFanLX.empty())
            return std::nullopt;
        return take(std::prev(listFanLX.end()));
    }

    // 按键删除结点，返回是否确实删掉了。
    bool erase(const Key &key)
    {
        const auto it = routeFanLX.find(key);
        if (it == routeFanLX.end())
            return false;
        unlinkFanLX(it->second);
        return true;
    }

    // 从队尾不断淘汰，直到条数与字节数都不超过给定上限。
    // 上限传 std::numeric_limits<std::size_t>::max() 表示该项不限制。
    void evictToFit(std::size_t maxEntries, std::size_t maxBytes = std::numeric_limits<std::size_t>::max())
    {
        while (!listFanLX.empty() && (listFanLX.size() > maxEntries || usedBytesFanLX > maxBytes))
            unlinkFanLX(std::prev(listFanLX.end())); // 队尾即最久未用，优先淘汰
    }

    // 清空整段：链表、路由表与字节账目一并归零。
    void clear() noexcept
    {
        listFanLX.clear();
        routeFanLX.clear();
        usedBytesFanLX = 0;
    }
};
