#include "tinalux/core/Reflect.h"

namespace tinalux::core {

TypeRegistry& TypeRegistry::instance()
{
    static TypeRegistry registry;
    return registry;
}

void TypeRegistry::registerType(TypeInfo info)
{
    auto name = info.name;
    types_.insert_or_assign(std::move(name), std::move(info));
}

const TypeInfo* TypeRegistry::findType(std::string_view name) const
{
    auto it = types_.find(name);
    if (it != types_.end())
        return &it->second;
    return nullptr;
}

void TypeRegistry::clear()
{
    types_.clear();
}

} // namespace tinalux::core
