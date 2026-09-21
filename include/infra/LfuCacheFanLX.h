// ============================================================================
// LfuCacheFanLX.h —— 独立的 LFU（Least Frequently Used，最不经常使用）频次控制器
// ----------------------------------------------------------------------------
// 作用：只维护"键 -> 访问次数"的映射，不缓存真正的 Value，因此可以像配件一样
//       组合进 ARC / LRU 等各类缓存容器中，由容器自己决定淘汰策略的其它部分。
// 频次规整（normalize）：
//   为防止个别热点键的访问次数无限增长、造成平均访问次数失衡，当所有键的
//   平均访问次数超过阈值 maxAverageAccessFanLX 时，把所有计数减半（(n+1)/2，
//   即向上取整），相当于对历史热度做一次"衰减/老化"。
// 设计要点（面向对象 / 泛型）：
//   - 模板参数 Key 与 Hash 可复用调用方的键类型及其哈希策略；
//   - 计数逻辑、规整逻辑全部封装在类内部，对外只暴露 touch / set / frequency
//     等语义清晰的接口。
// ============================================================================
#pragma once

#include <cstddef>
#include <limits>
#include <unordered_map>

// 独立的 LFU 频次控制器。
// 只保存 Key 与访问次数，不持有缓存 Value，便于与 ARC/LRU 等容器组合。
template <class Key, class Hash = std::hash<Key>>
class LfuFrequencyFanLX {
    // 哈希表：键 -> 累计访问次数
    std::unordered_map<Key, std::size_t, Hash> frequenciesFanLX;
    // 【增量维护的聚合量】所有计数的累加和，恒等于 frequenciesFanLX 中各值的总和。
    // 单独存一份是因为 averageAccessCount() 需要"总和 ÷ 个数"：若每次都现场遍历求和
    // 就是 O(n)，而调用它的 normalizeFanLX() 又挂在 touch/set 的每次调用末尾，会让
    // 缓存命中的复杂度退化成 O(n)。但总和其实只随四类修改而变（touch 的 +1、
    // set 的覆盖、erase 的删除、normalize 的减半），完全可以在修改发生的那一刻顺手更新。
    // 代价：每条修改路径都负有"同步累加和"的义务（本类所有修改入口均已照办），
    //       属典型的"空间换时间"。
    // 不变量：totalAccessFanLX == Σ frequenciesFanLX[i]，任何时刻都必须成立。
    std::size_t totalAccessFanLX = 0;
    // 平均访问次数的安全阈值：超过则触发整体衰减
    std::size_t maxAverageAccessFanLX = 32;

    // 内部工具：平均访问次数超阈值时，把所有键的计数统一减半，防止计数溢出式增长。
    // 减半是唯一无法增量维护累加和的操作（每个值的变化量各不相同），只能整表重扫一次；
    // 但本函数仅在平均值超限时才真正执行循环，触发频率低，这次 O(n) 扫描均摊后可忽略。
    void normalizeFanLX() {
        if (frequenciesFanLX.empty() || averageAccessCount() <= maxAverageAccessFanLX) return;
        totalAccessFanLX = 0;
        for (auto& item : frequenciesFanLX) {
            item.second = (item.second + 1) / 2;
            totalAccessFanLX += item.second; // 重算累加和，维持不变量
        }
    }

public:
    // 构造：允许外部指定平均访问次数的上限（0 保留默认 32）。
    explicit LfuFrequencyFanLX(std::size_t maxAverageAccess = 32)
        : maxAverageAccessFanLX(maxAverageAccess) {}

    // 记录一次访问：次数 +1（封顶在 size_t 最大值，防溢出），随后做衰减检查。
    void touch(const Key& key) {
        std::size_t& count = frequenciesFanLX[key]; // 新键会以 0 插入
        if (count < std::numeric_limits<std::size_t>::max()) {
            ++count;
            ++totalAccessFanLX; // 累加和同步 +1（计数已饱和时二者都不变）
        }
        normalizeFanLX();
    }

    // 直接设置某键的访问次数（绝对值覆盖，0 会被归一为 1），随后做衰减检查。
    // 与 touch 的区别：touch 是"又访问了一次"的增量语义，set 是"计数就是这个值"的
    // 绝对值语义，供调用方把别处已经算好的计数整体接管过来（ARC 晋升/衰减同步即用此途）。
    //
    // 【为什么 0 要抬成 1】三条理由：
    //   ① 语义不变量：能出现在这张表里的键，至少被访问过一次。而 frequency() 用返回值 0
    //      表示"键不存在"，若允许表内存 0，则"存在但计数为 0"与"不存在"无法区分。
    //      真正想清空某键的计数，应当调用 erase()，而不是 set(key, 0)。
    //   ② 衰减不动点：normalize 用 (n+1)/2 向上取半，n ≥ 1 时下界恒为 1；而 0 满足
    //      (0+1)/2 = 0，会永远卡死在 0，成为既洗不掉也长不起来的"死值"。
    //      在唯一能引入非法值的入口挡住它，等于在边界上守住这条下界。
    //   ③ LFU 排序语义：0 代表"从未访问"，等价于最冷，但这样的条目又实实在在占着
    //      一个表项，会让淘汰排序在"一堆 0 之间"做无意义的比较。
    //   注：现有调用点传入的值恒 ≥ 1（晋升处 freq ≥ k ≥ 1；衰减处 (n+1)/2 ≥ 1），
    //       故此处实际是一条防御式卫语句，防止未来的调用者塞进非法值。
    void set(const Key& key, std::size_t count) {
        const std::size_t value = count == 0 ? 1 : count;
        const std::size_t old = frequenciesFanLX[key]; // 键不存在则以 0 插入，旧值即 0
        frequenciesFanLX[key] = value;
        // 先减旧值再加新值：无符号数下若写成 value - old 会在"新值 < 旧值"时下溢，
        // 而 total ≥ old 恒成立，故 total - old 一定安全。
        totalAccessFanLX = totalAccessFanLX - old + value;
        normalizeFanLX();
    }

    // 查询某键当前的访问次数；键不存在时返回 0。
    std::size_t frequency(const Key& key) const noexcept {
        const auto it = frequenciesFanLX.find(key);
        return it == frequenciesFanLX.end() ? 0 : it->second;
    }

    // 删除某个键的频次记录（顺带把它的计数从累加和中扣除）。
    void erase(const Key& key) noexcept {
        const auto it = frequenciesFanLX.find(key);
        if (it == frequenciesFanLX.end()) return; // 键不存在则无事发生
        totalAccessFanLX -= it->second;           // 必须先扣掉累加和，再销毁结点
        frequenciesFanLX.erase(it);
    }
    // 清空全部频次记录（累加和一并归零，维持不变量）。
    void clear() noexcept {
        frequenciesFanLX.clear();
        totalAccessFanLX = 0;
    }
    // 当前记录的键数量。
    std::size_t size() const noexcept { return frequenciesFanLX.size(); }

    // 计算所有键的平均访问次数（empty 时返回 0，避免除零）。
    // 直接读取增量维护的 totalAccessFanLX，复杂度 O(1)，不再是原来的 O(n) 整表求和。
    // 结果与"现场遍历求和再整除"完全一致（整数除法的截断语义未变）。
    std::size_t averageAccessCount() const noexcept {
        return frequenciesFanLX.empty() ? 0 : totalAccessFanLX / frequenciesFanLX.size();
    }
};
