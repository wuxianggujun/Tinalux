#pragma once

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
