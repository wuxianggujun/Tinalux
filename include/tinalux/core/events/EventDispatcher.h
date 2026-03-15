#pragma once

// 注意：此文件当前未被使用，保留用于未来的事件分发重构。
// 当前事件分发路径仍以 Application / UIContext 中的直接分发为主。

#include <type_traits>
#include <utility>

#include "tinalux/core/events/Event.h"

namespace tinalux::core {

class EventDispatcher {
public:
    explicit EventDispatcher(Event& event)
        : event_(event)
    {
    }

    template <EventType TEventType, typename TEvent, typename F>
    bool dispatch(F&& handler)
    {
        static_assert(std::is_base_of_v<Event, TEvent>, "TEvent must derive from Event");

        if (event_.type() != TEventType) {
            return false;
        }

        event_.handled = event_.handled
            || static_cast<bool>(std::forward<F>(handler)(static_cast<TEvent&>(event_)));
        return true;
    }

private:
    Event& event_;
};

}  // namespace tinalux::core
