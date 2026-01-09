#pragma once

#include <utility>

#include <orteaf/internal/storage/cpu/cpu_storage_layout.h>
#include <orteaf/internal/execution/cpu/manager/cpu_buffer_manager.h>

namespace orteaf::internal::storage::cpu {
class CpuStorage {
public:
    using BufferLease = ::orteaf::internal::execution::cpu::manager::CpuBufferManager::BufferLease;
    using Layout = ::orteaf::internal::storage::cpu::CpuStorageLayout;

    struct Config {
        BufferLease buffer_lease{};
        Layout layout{};
    };

    CpuStorage() = default;
    explicit CpuStorage(Config config)
        : buffer_lease_(std::move(config.buffer_lease)),
          layout_(std::move(config.layout)) {}

    CpuStorage(const CpuStorage &) = default;
    CpuStorage &operator=(const CpuStorage &) = default;
    CpuStorage(CpuStorage &&) = default;
    CpuStorage &operator=(CpuStorage &&) = default;
    ~CpuStorage() = default;

private:
    BufferLease buffer_lease_;
    Layout layout_;
};
} // namespace orteaf::internal::storage::cpu
