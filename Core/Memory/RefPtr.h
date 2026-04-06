#ifndef CORE_REFPTR_H
#define CORE_REFPTR_H

#include <cassert>
#include <utility>

#include "CoreGlobal.h"
#include "WeakRefPtr.h"

CORE_NAMESPACE_BEGIN

template <class T>
class RefPtr {
public:
    RefPtr() : m_ptr(nullptr) {}
    RefPtr(T* t) : m_ptr(t)
    {
        if (m_ptr) {
            m_ptr->ref();
        }
    }
    RefPtr(const RefPtr& rp) : m_ptr(rp.m_ptr)
    {
        if (m_ptr) {
            m_ptr->ref();
        }
    }
    RefPtr(RefPtr&& rp) noexcept : m_ptr(rp.m_ptr) { rp.m_ptr = nullptr; }
    ~RefPtr()
    {
        if (m_ptr) {
            m_ptr->unref();
        }
        m_ptr = nullptr;
    }

    RefPtr(const WeakRefPtr<T>& weak_ptr)
    {
        if (weak_ptr.isAlive()) {
            m_ptr = weak_ptr.ptr();
        }
        else {
            m_ptr = nullptr;
        }
        if (m_ptr) {
            m_ptr->ref();
        }
    }

    RefPtr(WeakRefPtr<T>& weak_ptr)
    {
        if (weak_ptr.isAlive()) {
            m_ptr = weak_ptr.ptr();
        }
        else {
            m_ptr = nullptr;
        }
        if (m_ptr) {
            m_ptr->ref();
        }
    }

    RefPtr& operator=(const RefPtr& rp)
    {
        if (m_ptr != rp.m_ptr) {
            T* tmpm_ptr = m_ptr;
            m_ptr       = rp.m_ptr;
            if (m_ptr) {
                m_ptr->ref();
            }
            if (tmpm_ptr) {
                tmpm_ptr->unref();
            }
        }
        return *this;
    }

    RefPtr& operator=(RefPtr&& rp) noexcept
    {
        if (this != &rp) {
            if (m_ptr) {
                m_ptr->unref();
            }
            m_ptr    = rp.m_ptr;
            rp.m_ptr = nullptr;
        }
        return *this;
    }

    RefPtr& operator=(T* ptr)
    {
        if (m_ptr != ptr) {
            T* tmpm_ptr = m_ptr;
            m_ptr       = ptr;
            if (m_ptr) {
                m_ptr->ref();
            }
            if (tmpm_ptr) {
                tmpm_ptr->unref();
            }
        }
        return *this;
    }

    bool operator==(const RefPtr& rp) const { return (m_ptr == rp.m_ptr); }

    bool operator==(const T* ptr) const { return (m_ptr == ptr); }

    bool operator!=(const RefPtr& rp) const { return (m_ptr != rp.m_ptr); }

    bool operator!=(const T* ptr) const { return (m_ptr != ptr); }

    bool operator<(const RefPtr& rp) const { return (m_ptr < rp.m_ptr); }

    bool operator>(const RefPtr& rp) const { return (m_ptr > rp.m_ptr); }

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
            if (m_ptr) {
                m_ptr->unref();
            }
            m_ptr = ptr;
            if (m_ptr) {
                m_ptr->ref();
            }
        }
    }

private:
    T* m_ptr;
};

CORE_NAMESPACE_END

#endif    // CORE_REFPTR_H
