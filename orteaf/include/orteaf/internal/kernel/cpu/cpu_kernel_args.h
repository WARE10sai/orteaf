#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

#include <orteaf/internal/kernel/access.h>
#include <orteaf/internal/kernel/kernel_key.h>
#include <orteaf/internal/storage/registry/storage_types.h>

namespace orteaf::internal::kernel::cpu {

/**
 * @brief CPU kernel arguments container.
 */
class CpuKernelArgs {
public:
  using StorageLease = ::orteaf::internal::storage::CpuStorageLease;

  static constexpr std::size_t kMaxBindings = 16;
  static constexpr std::size_t kParamBytes = 1024;

  template <typename Params>
  bool setParams(const Params &params, KernelKey kernel_key) {
    static_assert(std::is_trivially_copyable_v<Params>,
                  "Params must be trivially copyable.");
    return setParamsRaw(&params, sizeof(Params), kernel_key);
  }

  template <typename Params>
  bool getParams(Params &out, KernelKey expected_key) const {
    static_assert(std::is_trivially_copyable_v<Params>,
                  "Params must be trivially copyable.");
    if (kernel_key_ != expected_key || params_size_ != sizeof(Params)) {
      return false;
    }
    std::memcpy(&out, params_.data(), sizeof(Params));
    return true;
  }

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

  bool setParamsRaw(const void *data, std::size_t size, KernelKey kernel_key) {
    if (size > kParamBytes) {
      return false;
    }
    params_size_ = size;
    kernel_key_ = kernel_key;
    if (size > 0) {
      std::memcpy(params_.data(), data, size);
    }
    return true;
  }

  std::size_t paramsSize() const { return params_size_; }
  KernelKey kernelKey() const { return kernel_key_; }

  std::size_t paramsCapacity() const { return kParamBytes; }

  const std::byte *paramsData() const { return params_.data(); }
  std::byte *paramsData() { return params_.data(); }

private:
  std::array<StorageLease, kMaxBindings> storage_leases_{};
  std::array<Access, kMaxBindings> storage_accesses_{};
  std::size_t storage_count_{0};
  alignas(std::max_align_t) std::array<std::byte, kParamBytes> params_{};
  std::size_t params_size_{0};
  KernelKey kernel_key_{};
};

} // namespace orteaf::internal::kernel::cpu
