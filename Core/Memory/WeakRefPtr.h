#ifndef CORE_WEAKREFPTR_H
#define CORE_WEAKREFPTR_H

#include <cassert>
#include <utility>

#include "ControlBlock.h"
#include "CoreGlobal.h"

CORE_NAMESPACE_BEGIN

template <class T>
class RefPtr;

template <class T>
class WeakRefPtr {
public:
    WeakRefPtr() : m_ptr(nullptr), m_control_block(nullptr) {}
    WeakRefPtr(T* t) : m_ptr(t), m_control_block(t ? t->m_control_block : nullptr)
    {
        if (isAlive()) {
            m_control_block->weakRef();
        }
    }
    WeakRefPtr(const WeakRefPtr& rp) : m_ptr(rp.m_ptr), m_control_block(rp.m_control_block)
    {
        if (isAlive()) {
            m_control_block->weakRef();
        }
    }
    WeakRefPtr(WeakRefPtr&& rp) noexcept : m_ptr(rp.m_ptr), m_control_block(rp.m_control_block)
    {
        rp.m_ptr           = nullptr;
        rp.m_control_block = nullptr;
    }

    WeakRefPtr(const RefPtr<T>& ref_ptr)
    {
        m_ptr = ref_ptr.ptr();
        if (m_ptr) {
            m_control_block = m_ptr->m_control_block;
        }
        else {
            m_control_block = nullptr;
        }

        if (isAlive()) {
            m_control_block->weakRef();
        }
    }

    WeakRefPtr(RefPtr<T>& ref_ptr)
    {
        m_ptr = ref_ptr.ptr();
        if (m_ptr) {
            m_control_block = m_ptr->m_control_block;
        }
        else {
            m_control_block = nullptr;
        }

        if (isAlive()) {
            m_control_block->weakRef();
        }
    }

    ~WeakRefPtr()
    {
        if (isAlive()) {
            m_control_block->weakUnref();
        }
        m_ptr           = nullptr;
        m_control_block = nullptr;
    }

    WeakRefPtr& operator=(const WeakRefPtr& rp)
    {
        if (m_ptr != rp.m_ptr) {
            if (isAlive()) {
                m_control_block->weakUnref();
            }
            setPtr(rp);
            if (isAlive()) {
                m_control_block->weakRef();
            }
        }
        return *this;
    }

    WeakRefPtr& operator=(WeakRefPtr&& rp) noexcept
    {
        if (this != &rp) {
            if (isAlive()) {
                m_control_block->weakUnref();
            }
            setPtr(rp);
            rp.setPtr(nullptr);
        }
        return *this;
    }

    WeakRefPtr& operator=(T* ptr)
    {
        if (m_ptr != ptr) {
            if (isAlive()) {
                m_control_block->weakUnref();
            }
            setPtr(ptr);
            if (isAlive()) {
                m_control_block->weakRef();
            }
        }
        return *this;
    }

    bool operator==(const WeakRefPtr& rp) const { return (m_ptr == rp.m_ptr); }

    bool operator==(const T* ptr) const { return (m_ptr == ptr); }

    bool operator!=(const WeakRefPtr& rp) const { return (m_ptr != rp.m_ptr); }

    bool operator!=(const T* ptr) const { return (m_ptr != ptr); }

    bool operator<(const WeakRefPtr& rp) const { return (m_ptr < rp.m_ptr); }

    bool operator>(const WeakRefPtr& rp) const { return (m_ptr > rp.m_ptr); }

    bool operator>(const T* ptr) const { return (m_ptr > ptr); }

    T& operator*()
    {
        assert(m_ptr != nullptr);
        return *m_ptr;
    }

    const T& operator*() const
    {
        assert(m_ptr != nullptr);
        return *m_ptr;
    }

    T* operator->()
    {
        assert(m_ptr != nullptr);
        return m_ptr;
    }

    const T* operator->() const
    {
        assert(m_ptr != nullptr);
        return m_ptr;
    }

    bool operator!() const { return m_ptr == nullptr; }

    bool valid() const { return m_ptr != nullptr; }

    T* ptr() { return m_ptr; }

    const T* ptr() const { return m_ptr; }

    void reset(T* ptr = nullptr)
    {
        if (m_ptr != ptr) {
            if (isAlive()) {
                m_control_block->weakUnref();
            }
            setPtr(ptr);
            if (isAlive()) {
                m_control_block->weakRef();
            }
        }
    }

    bool isAlive() const { return (m_control_block && m_control_block->strongCount() > 0); }

protected:
    void setPtr(T* ptr)
    {
        if (ptr) {
            m_ptr           = ptr;
            m_control_block = ptr->m_control_block;    /// 安全性のため別保持
        }
        else {
            m_ptr           = nullptr;
            m_control_block = nullptr;
        }
    }
    void setPtr(const WeakRefPtr& rp)
    {
        m_ptr           = rp.m_ptr;
        m_control_block = rp.m_control_block;    /// 安全性のため別保持
    }

private:
    T*            m_ptr;
    ControlBlock* m_control_block;
};

CORE_NAMESPACE_END

#endif    // CORE_WEAKREFPTR_H
