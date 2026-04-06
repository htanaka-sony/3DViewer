#ifndef CORE_CONTROLBLOCK_H
#define CORE_CONTROLBLOCK_H

#include <functional>
#include "CoreGlobal.h"

#include <cassert>

#define ENABLE_REFPTR_THREAD_SAFE 1

#ifdef ENABLE_REFPTR_THREAD_SAFE
    #include <atomic>
#endif

CORE_NAMESPACE_BEGIN

class ControlBlock {
    friend class Referenced;

    /// 基本Referencedのみアクセス
protected:
    ControlBlock() : m_strong_count(0), m_weak_count(0) {}

    void addStrong()
    {
#ifdef ENABLE_REFPTR_THREAD_SAFE
        m_strong_count.fetch_add(1, std::memory_order_relaxed);
#else
        ++m_strong_count;
#endif
    }

    void releaseStrong()
    {
#ifdef ENABLE_REFPTR_THREAD_SAFE
        m_strong_count.fetch_sub(1, std::memory_order_relaxed);
#else
        --m_strong_count;
#endif
    }

    void addWeak()
    {
#ifdef ENABLE_REFPTR_THREAD_SAFE
        m_weak_count.fetch_add(1, std::memory_order_relaxed);
#else
        ++m_weak_count;
#endif
    }

    void releaseWeak()
    {
#ifdef ENABLE_REFPTR_THREAD_SAFE
        m_weak_count.fetch_sub(1, std::memory_order_relaxed);
#else
        --m_weak_count;
#endif
    }

    int weakCount() const
    {
#ifdef ENABLE_REFPTR_THREAD_SAFE
        return m_weak_count.load(std::memory_order_relaxed);
#else
        return m_weak_count;
#endif
    }

    /// WeakRefPtrでアクセス
public:
    int strongCount() const
    {
#ifdef ENABLE_REFPTR_THREAD_SAFE
        return m_strong_count.load(std::memory_order_relaxed);
#else
        return m_strong_count;
#endif
    }

    void weakRef() { addWeak(); }

    void weakUnref()
    {
        releaseWeak();
        if (strongCount() == 0 && weakCount() == 0) {
            delete this;
        }
    }

private:
#ifdef ENABLE_REFPTR_THREAD_SAFE
    std::atomic<int> m_strong_count;    /// 強参照カウント
    std::atomic<int> m_weak_count;      /// 弱参照カウント
#else
    int m_strong_count;    /// 強参照カウント
    int m_weak_count;      /// 弱参照カウント
#endif
};

CORE_NAMESPACE_END

#endif    // CORE_CONTROLBLOCK_H
