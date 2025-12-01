#pragma once

#include <utility>

namespace orteaf::internal::base {

/**
 * @brief RAII handle wrapper that pairs a Handle with a cached resource value.
 *
 * Construction is restricted to the Manager type (friend). Managers call the
 * private ctor with the acquired resource; destruction releases via Manager::release.
 */
template <class HandleT, class ResourceT, class ManagerT>
class HandleScope {
    friend ManagerT;

public:
    HandleScope(const HandleScope&) = delete;
    HandleScope& operator=(const HandleScope&) = delete;

    HandleScope(HandleScope&& other) noexcept
        : manager_(std::exchange(other.manager_, nullptr)),
          handle_(std::move(other.handle_)),
          resource_(std::move(other.resource_)) {}

    HandleScope& operator=(HandleScope&& other) noexcept {
        if (this != &other) {
            release();
            manager_ = std::exchange(other.manager_, nullptr);
            handle_ = std::move(other.handle_);
            resource_ = std::move(other.resource_);
        }
        return *this;
    }

    ~HandleScope() { release(); }

    const HandleT& handle() const noexcept { return handle_; }

    ResourceT& get() noexcept { return resource_; }
    const ResourceT& get() const noexcept { return resource_; }

    ResourceT* operator->() noexcept { return std::addressof(resource_); }
    const ResourceT* operator->() const noexcept { return std::addressof(resource_); }

private:
    HandleScope(ManagerT* mgr, HandleT handle, ResourceT resource) noexcept
        : manager_(mgr), handle_(std::move(handle)), resource_(std::move(resource)) {}

    void release() noexcept {
        if (manager_) {
            manager_->release(handle_);
            manager_ = nullptr;
        }
    }

    ManagerT* manager_{nullptr};
    HandleT handle_{};
    ResourceT resource_{};
};

}  // namespace orteaf::internal::base
