#ifndef CORE_ATTRIBUTE_H
#define CORE_ATTRIBUTE_H

#include "CoreConstants.h"
#include "CoreGlobal.h"

#include <any>
#include <map>
#include <string>
#include <typeinfo>

CORE_NAMESPACE_BEGIN

template <class T>
class Attribute {
public:
    void setAttribute(const T& key, const std::any& value);

    void setAttributes(const std::map<T, std::any>& attributes);

    const std::any& attribute(const T& key) const;

    bool hasAttribute(const T& key) const;

    void removeAttribute(const T& key);

    void removeAll();

    const std::type_info& attributeType(const std::any& value) const;

    const std::type_info& attributeType(const T& key) const;

    const std::map<T, std::any>& attributes() const;

private:
    std::map<T, std::any> m_attributes;
};

extern template class Attribute<AttributeType>;
extern template class Attribute<std::wstring>;

CORE_NAMESPACE_END

#endif    // CORE_ATTRIBUTE_H
