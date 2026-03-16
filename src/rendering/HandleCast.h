#pragma once

#include <cstdint>
#include <type_traits>

namespace tinalux::rendering {

/// Type-safe cast from a native handle (pointer or integer) to an opaque void*.
template <typename Handle>
void* opaqueHandle(Handle handle)
{
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<void*>(handle);
    } else {
        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(handle));
    }
}

/// Type-safe cast from an opaque void* back to the original handle type.
template <typename Handle>
Handle handleFromOpaque(void* handle)
{
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<Handle>(handle);
    } else {
        return static_cast<Handle>(reinterpret_cast<std::uintptr_t>(handle));
    }
}

}  // namespace tinalux::rendering
