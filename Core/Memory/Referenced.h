#ifndef CORE_REFERENCED_H
#define CORE_REFERENCED_H

#include "ControlBlock.h"
#include "CoreGlobal.h"

CORE_NAMESPACE_BEGIN

class CORE_EXPORT Referenced {
    template <class T>
    friend class RefPtr;
    template <class T>
    friend class WeakRefPtr;

public:
    void ref() const;
    void unref() const;
    int  refCount() const;

protected:
    Referenced();
    virtual ~Referenced();

    int weakRefCount() const;

private:
    mutable ControlBlock* m_control_block;
};

CORE_NAMESPACE_END

#endif    // CORE_REFERENCED_H
