#pragma once

#include <orteaf/internal/storage/cpu/cpu_storage_layout.h>
#include <orteaf/internal/execution/cpu/manager/cpu_buffer_manager.h>

namespace orteaf::internal::storage::cpu {
class CpuStorage {
public:
    using BufferLease = ::orteaf::internal::execution::cpu::manager::CpuBufferManager::BufferLease;
    using Layout = ::orteaf::internal::storage::cpu::CpuStorageLayout;

private:
    BufferLease buffer_lease_;
    Layout layout_;
};
} // namespace orteaf::internal::storage::cpu