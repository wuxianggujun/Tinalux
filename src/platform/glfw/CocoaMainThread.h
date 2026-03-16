#pragma once

#if defined(__APPLE__)

#include <atomic>

#import <Cocoa/Cocoa.h>
#import <dispatch/dispatch.h>

#include "tinalux/core/Log.h"

namespace tinalux::platform::detail {

inline void logMainThreadHandoffOnce(
    std::atomic<bool>& guard,
    const char* component,
    const char* operation)
{
    bool expected = false;
    if (guard.compare_exchange_strong(expected, true)) {
        core::logWarnCat(
            "platform",
            "{} '{}' was invoked off the main thread; dispatching to the main thread",
            component,
            operation);
    }
}

inline void runOnMainThread(
    const char* component,
    const char* operation,
    std::atomic<bool>& handoffGuard,
    dispatch_block_t block)
{
    if ([NSThread isMainThread]) {
        block();
        return;
    }

    logMainThreadHandoffOnce(handoffGuard, component, operation);
    dispatch_sync(dispatch_get_main_queue(), block);
}

template <typename T>
inline T runValueOnMainThread(
    const char* component,
    const char* operation,
    std::atomic<bool>& handoffGuard,
    T (^block)())
{
    if ([NSThread isMainThread]) {
        return block();
    }

    logMainThreadHandoffOnce(handoffGuard, component, operation);
    __block T value {};
    dispatch_sync(dispatch_get_main_queue(), ^{
        value = block();
    });
    return value;
}

}  // namespace tinalux::platform::detail

#endif
