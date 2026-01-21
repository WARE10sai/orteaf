#pragma once

#if ORTEAF_ENABLE_MPS

#include <array>
#include <cstddef>
#include <cstring>
#include <utility>

#include <orteaf/internal/kernel/access.h>
#include <orteaf/internal/storage/registry/storage_types.h>

namespace orteaf::internal::kernel::mps {

/**
 * @brief MPS kernel arguments container.
 */
class MpsKernelArgs {
public:
  using StorageLease = ::orteaf::internal::storage::MpsStorageLease;

  static constexpr std::size_t kMaxBindings = 16;
  static constexpr std::size_t kParamBytes = 1024;

  void addStorageLease(StorageLease lease, Access access) {
    if (storage_count_ >= kMaxBindings) {
      return;
    }
    storage_leases_[storage_count_] = std::move(lease);
    storage_accesses_[storage_count_] = access;
    ++storage_count_;
  }

  std::size_t storageCount() const { return storage_count_; }

  std::size_t storageCapacity() const { return kMaxBindings; }

  const StorageLease &storageLeaseAt(std::size_t index) const {
    return storage_leases_[index];
  }

  Access storageAccessAt(std::size_t index) const {
    return storage_accesses_[index];
  }

  void clearStorages() {
    for (std::size_t i = 0; i < storage_count_; ++i) {
      storage_leases_[i] = StorageLease{};
      storage_accesses_[i] = Access::None;
    }
    storage_count_ = 0;
  }

  bool setParams(const void *data, std::size_t size) {
    if (size > kParamBytes) {
      return false;
    }
    params_size_ = size;
    if (size > 0) {
      std::memcpy(params_.data(), data, size);
    }
    return true;
  }

  std::size_t paramsSize() const { return params_size_; }

  std::size_t paramsCapacity() const { return kParamBytes; }

  const std::byte *paramsData() const { return params_.data(); }
  std::byte *paramsData() { return params_.data(); }

private:
  std::array<StorageLease, kMaxBindings> storage_leases_{};
  std::array<Access, kMaxBindings> storage_accesses_{};
  std::size_t storage_count_{0};
  alignas(std::max_align_t) std::array<std::byte, kParamBytes> params_{};
  std::size_t params_size_{0};
};

} // namespace orteaf::internal::kernel::mps

#endif // ORTEAF_ENABLE_MPS
