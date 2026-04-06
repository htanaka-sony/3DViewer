#include "Attribute.h"

CORE_NAMESPACE_BEGIN

template <class T>
void Attribute<T>::setAttribute(const T& key, const std::any& value)
{
    m_attributes[key] = value;
}

template <class T>
void Attribute<T>::setAttributes(const std::map<T, std::any>& attributes)
{
    m_attributes = attributes;
}

template <class T>
const std::any& Attribute<T>::attribute(const T& key) const
{
    auto it = m_attributes.find(key);
    if (it != m_attributes.end()) {
        return it->second;
    }
    else {
        static const std::any empty_attr;
        return empty_attr;
    }
}

template <class T>
bool Attribute<T>::hasAttribute(const T& key) const
{
    return m_attributes.find(key) != m_attributes.end();
}

template <class T>
void Attribute<T>::removeAttribute(const T& key)
{
    m_attributes.erase(key);
}

template <class T>
void Attribute<T>::removeAll()
{
    m_attributes.clear();
}

template <class T>
const std::type_info& Attribute<T>::attributeType(const std::any& value) const
{
    return value.type();
}

template <class T>
const std::type_info& Attribute<T>::attributeType(const T& key) const
{
    auto it = m_attributes.find(key);
    if (it != m_attributes.end()) {
        return it->second.type();
    }
    else {
        return typeid(void);
    }
}

template <class T>
const std::map<T, std::any>& Attribute<T>::attributes() const
{
    return m_attributes;
}

/// 明示的なインスタンス化
template class Attribute<AttributeType>;
template class Attribute<std::wstring>;

CORE_NAMESPACE_END
