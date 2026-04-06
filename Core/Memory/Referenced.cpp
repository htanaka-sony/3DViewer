#include "Referenced.h"
#include "cassert"

CORE_NAMESPACE_BEGIN

Referenced::Referenced() : m_control_block(nullptr) {}

Referenced::~Referenced()
{
    assert(!m_control_block || refCount() == 0);
}

void Referenced::ref()
{
    if (!m_control_block) {
        m_control_block = new ControlBlock;
    }
    m_control_block->addStrong();
}

void Referenced::unref()
{
    if (!m_control_block) {
        return;
    }
    m_control_block->releaseStrong();
    if (m_control_block->strongCount() == 0) {
        if (m_control_block->weakCount() == 0) {
            delete m_control_block;
            m_control_block = nullptr;
        }
        delete this;
    }
}

int Referenced::refCount() const
{
    if (!m_control_block) {
        return 0;
    }
    return m_control_block->strongCount();
}

int Referenced::weakRefCount() const
{
    if (!m_control_block) {
        return 0;
    }
    return m_control_block->weakCount();
}

CORE_NAMESPACE_END
