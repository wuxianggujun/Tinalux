#include "tinalux/ui/ResourceLoader.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <thread>
#include <unordered_map>
#include <utility>

#include "tinalux/core/Log.h"
#include "tinalux/ui/ResourceManager.h"

namespace tinalux::ui {

struct ResourceLoaderImpl {
    struct ImageJob {
        std::string path;
        std::shared_ptr<detail::ResourceState<rendering::Image>> state;
        std::uint64_t generation = 0;
    };

    std::mutex mutex;
    std::condition_variable cv;
    std::deque<ImageJob> jobs;
    std::unordered_map<std::string, std::weak_ptr<detail::ResourceState<rendering::Image>>> inFlight;
    std::vector<std::shared_ptr<detail::ResourceState<rendering::Image>>> completed;
    std::thread worker;
    std::atomic<bool> stopping = false;
    std::uint64_t generation = 1;
};

namespace {

void clearStateCallbacks(const std::shared_ptr<detail::ResourceState<rendering::Image>>& state)
{
    if (state == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->callbacks.clear();
}

void workerLoop(ResourceLoaderImpl& impl)
{
    for (;;) {
        ResourceLoaderImpl::ImageJob job;
        {
            std::unique_lock<std::mutex> lock(impl.mutex);
            impl.cv.wait(lock, [&impl] {
                return impl.stopping.load() || !impl.jobs.empty();
            });

            if (impl.stopping.load() && impl.jobs.empty()) {
                return;
            }

            job = std::move(impl.jobs.front());
            impl.jobs.pop_front();
        }

        const rendering::Image image = rendering::loadImageFromFile(job.path);
        {
            std::lock_guard<std::mutex> stateLock(job.state->mutex);
            job.state->ready = true;
            job.state->value = image;
        }

        std::lock_guard<std::mutex> lock(impl.mutex);
        const bool generationMatches = job.generation == impl.generation;
        if (const auto it = impl.inFlight.find(job.path); it != impl.inFlight.end()) {
            if (it->second.lock() == job.state) {
                impl.inFlight.erase(it);
            }
        }

        if (generationMatches) {
            impl.completed.push_back(job.state);
        } else {
            clearStateCallbacks(job.state);
        }
    }
}

}  // namespace

ResourceLoader& ResourceLoader::instance()
{
    static ResourceLoader loader;
    return loader;
}

ResourceLoader::ResourceLoader()
    : impl_(std::make_unique<ResourceLoaderImpl>())
{
    impl_->worker = std::thread([this] {
        workerLoop(*impl_);
    });
}

ResourceLoader::~ResourceLoader()
{
    if (impl_ == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->stopping = true;
    }
    impl_->cv.notify_all();
    if (impl_->worker.joinable()) {
        impl_->worker.join();
    }
}

ResourceHandle<rendering::Image> ResourceLoader::loadImageAsync(const std::string& path)
{
    const std::string resolvedPath = ResourceManager::instance().resolveImagePath(path);
    if (resolvedPath.empty()) {
        auto state = std::make_shared<detail::ResourceState<rendering::Image>>();
        state->ready = true;
        return ResourceHandle<rendering::Image>(std::move(state));
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (const auto it = impl_->inFlight.find(resolvedPath); it != impl_->inFlight.end()) {
        if (const auto existing = it->second.lock(); existing != nullptr) {
            return ResourceHandle<rendering::Image>(existing);
        }
        impl_->inFlight.erase(it);
    }

    auto state = std::make_shared<detail::ResourceState<rendering::Image>>();
    impl_->inFlight[resolvedPath] = state;
    impl_->jobs.push_back({
        .path = resolvedPath,
        .state = state,
        .generation = impl_->generation,
    });
    impl_->cv.notify_one();
    core::logDebugCat("ui", "Queued async image load '{}'", resolvedPath);
    return ResourceHandle<rendering::Image>(std::move(state));
}

void ResourceLoader::preload(const std::vector<std::string>& paths)
{
    for (const std::string& path : paths) {
        if (!path.empty()) {
            (void)loadImageAsync(path);
        }
    }
}

bool ResourceLoader::pumpCallbacks()
{
    std::vector<std::shared_ptr<detail::ResourceState<rendering::Image>>> completed;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        completed.swap(impl_->completed);
    }

    bool invokedCallbacks = !completed.empty();
    for (const auto& state : completed) {
        std::vector<std::function<void(const rendering::Image&)>> callbacks;
        rendering::Image value;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            callbacks = std::move(state->callbacks);
            value = state->value;
        }

        for (auto& callback : callbacks) {
            if (!callback) {
                continue;
            }
            callback(value);
            invokedCallbacks = true;
        }
    }

    return invokedCallbacks;
}

void ResourceLoader::clear()
{
    std::deque<ResourceLoaderImpl::ImageJob> jobs;
    std::unordered_map<std::string, std::weak_ptr<detail::ResourceState<rendering::Image>>> inFlight;
    std::vector<std::shared_ptr<detail::ResourceState<rendering::Image>>> completed;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        ++impl_->generation;
        jobs.swap(impl_->jobs);
        inFlight.swap(impl_->inFlight);
        completed.swap(impl_->completed);
    }

    for (auto& job : jobs) {
        clearStateCallbacks(job.state);
    }
    for (const auto& [_, weakState] : inFlight) {
        clearStateCallbacks(weakState.lock());
    }
    for (const auto& state : completed) {
        clearStateCallbacks(state);
    }
}

}  // namespace tinalux::ui
