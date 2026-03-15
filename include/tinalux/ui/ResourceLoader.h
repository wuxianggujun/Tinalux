#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "tinalux/rendering/rendering.h"

namespace tinalux::ui {

namespace detail {

template<typename T>
struct ResourceState {
    mutable std::mutex mutex;
    bool ready = false;
    T value {};
    std::vector<std::function<void(const T&)>> callbacks;
};

}  // namespace detail

struct ResourceLoaderImpl;

template<typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;

    explicit operator bool() const
    {
        return state_ != nullptr;
    }

    bool isReady() const
    {
        if (state_ == nullptr) {
            return false;
        }

        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->ready;
    }

    const T& get() const
    {
        static const T empty {};
        if (state_ == nullptr) {
            return empty;
        }

        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->ready ? state_->value : empty;
    }

    void onReady(std::function<void(const T&)> callback) const
    {
        if (state_ == nullptr || !callback) {
            return;
        }

        T value {};
        bool ready = false;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (!state_->ready) {
                state_->callbacks.push_back(std::move(callback));
                return;
            }
            value = state_->value;
            ready = true;
        }

        if (ready) {
            callback(value);
        }
    }

private:
    explicit ResourceHandle(std::shared_ptr<detail::ResourceState<T>> state)
        : state_(std::move(state))
    {
    }

    std::shared_ptr<detail::ResourceState<T>> state_;

    friend class ResourceLoader;
};

class ResourceLoader {
public:
    static ResourceLoader& instance();

    ResourceHandle<rendering::Image> loadImageAsync(const std::string& path);
    void preload(const std::vector<std::string>& paths);
    bool pumpCallbacks();
    void clear();

private:
    ResourceLoader();
    ~ResourceLoader();
    ResourceLoader(const ResourceLoader&) = delete;
    ResourceLoader& operator=(const ResourceLoader&) = delete;

    std::unique_ptr<ResourceLoaderImpl> impl_;
};

}  // namespace tinalux::ui
