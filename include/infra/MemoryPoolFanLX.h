// ============================================================================
// MemoryPoolFanLX.h —— 内存池（对象池）：批量分配 + 空闲链表复用
// ----------------------------------------------------------------------------
// 动机：IM 系统中高频创建 / 销毁的小对象（如消息体、连接上下文、好友记录）
//       直接走 ::operator new / delete 会产生大量系统调用与内存碎片。
//       内存池一次性向系统申请大块内存（BlockSize 个结点为一块），
//       用"空闲链表"（freeListFanLX）管理可复用结点：
//         · create：从空闲链表头部取一个结点，用 placement new 在结点存储区内构造对象；
//         · destroy：先调用对象析构函数，再把结点归还空闲链表头部。
//       整块内存由块链表（blocksFanLX）登记，析构函数统一回收，绝不泄漏。
// 关键点：
//   - NodeFanLX 的 storage 用 alignas(T) 保证满足任意 T 的对齐要求；
//   - 只在对象构造/析构层做干预，不重载全局 new，因此不影响其它代码。
// ============================================================================
#pragma once
#include <cstddef>
#include <new>
#include <utility>

// 通用内存池模板：T 为对象类型，BlockSize 为每批申请的结点个数。
template <class T, std::size_t BlockSize = 256>
class MemoryPoolFanLX
{
    // 结点：既是"空闲链表"的一环，也内嵌对象存储区。
    struct NodeFanLX
    {
        NodeFanLX *next;                      // 空闲链表后继指针（空闲时才有效）
        alignas(T) std::byte storage[sizeof(T)]; // 对象存放区：按 T 对齐并预留足够空间
    };

    NodeFanLX *freeListFanLX = nullptr; // 空闲链表头：create 从这里取结点
    NodeFanLX *blocksFanLX = nullptr;   // 已申请大块内存链表头：析构时统一释放

    // 内部工具：向系统申请一块可容纳 BlockSize 个结点的大内存，
    // 登记到 blocksFanLX，并把其中除首结点外的结点串成空闲链表。
    void growFanLX()
    {
        auto *block = static_cast<NodeFanLX *>(::operator new[](sizeof(NodeFanLX) * BlockSize));
        block[0].next = blocksFanLX; // 块首结点兼任块链表结点（存下一块地址）
        blocksFanLX = block;
        for (std::size_t i = 1; i < BlockSize; ++i)
        {
            block[i].next = freeListFanLX; // 其余结点逐个压入空闲链表
            freeListFanLX = &block[i];
        }
    }

public:
    MemoryPoolFanLX() = default; // 初始为空，首次 create 时才真正申请内存（懒分配）

    // 析构：沿块链表释放所有申请过的大块内存。
    ~MemoryPoolFanLX()
    {
        while (blocksFanLX)
        {
            auto *next = blocksFanLX[0].next; // 先记住下一块，防止释放后访问
            ::operator delete[](blocksFanLX);
            blocksFanLX = next;
        }
    }

    // 构造对象：从空闲链表取结点，用可变参数模板 + placement new 就地构造 T，
    // 返回指向新对象的指针（T 的构造函数参数原样转发）。
    template <class... Args>
    T *create(Args &&...args)
    {
        if (!freeListFanLX)
            growFanLX(); // 空闲链表为空：先申请新内存块
        auto *node = freeListFanLX;     // 摘下空闲链表头结点
        freeListFanLX = node->next;
        return ::new (node->storage) T(std::forward<Args>(args)...); // placement new 构造
    }

    // 析构对象：调用析构函数，再把该结点归还空闲链表以便复用。
    void destroy(T *object) noexcept
    {
        if (!object)
            return; // 空指针无需处理
        object->~T(); // 手动调用析构（对象占用的内存来自池子，不能 delete）
        // 由对象指针反算结点地址：对象存放在 NodeFanLX::storage 中
        auto *node = reinterpret_cast<NodeFanLX *>(reinterpret_cast<std::byte *>(object) - offsetof(NodeFanLX, storage));
        node->next = freeListFanLX; // 头插回空闲链表
        freeListFanLX = node;
    }
};
