#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

#if ORTEAF_ENABLE_MPS
#include "orteaf/internal/execution/mps/manager/mps_kernel_metadata_manager.h"
#endif
#include "orteaf/internal/kernel/core/kernel_entry.h"

namespace orteaf::internal::kernel::core {

class KernelMetadataLease;

namespace detail {

template <class LeaseT>
concept KernelMetadataPayloadRebuildable = requires(
    const LeaseT &lease,
    ::orteaf::internal::kernel::core::KernelEntry &entry) {
  { static_cast<bool>(lease) } -> std::same_as<bool>;
  { lease.operator->()->rebuildKernelEntry(entry) } -> std::same_as<void>;
};

template <class LeaseT>
concept KernelMetadataFromEntryBuildable =
    requires(const LeaseT &lease) {
      { static_cast<bool>(lease) } -> std::same_as<bool>;
    } && requires(
             const LeaseT &lease_value,
             ::orteaf::internal::kernel::core::KernelEntry::ExecuteFunc execute) {
      { ::orteaf::internal::execution::mps::resource::MpsKernelMetadata::
            buildMetadataLeaseFromBase(*lease_value.operator->(), execute) } ->
          std::same_as<::orteaf::internal::kernel::core::KernelMetadataLease>;
    };

} // namespace detail

/**
 * @brief Type-erased kernel metadata lease.
 */
class KernelMetadataLease {
public:
#if ORTEAF_ENABLE_MPS
  using MpsKernelMetadataLease =
      ::orteaf::internal::execution::mps::manager::MpsKernelMetadataManager::
          KernelMetadataLease;
#endif

  using Variant = std::variant<
      std::monostate
#if ORTEAF_ENABLE_MPS
      ,
      MpsKernelMetadataLease
#endif
      >;

  KernelMetadataLease() = default;

  explicit KernelMetadataLease(Variant lease) noexcept
      : lease_(std::move(lease)) {}

  Variant &lease() noexcept { return lease_; }
  const Variant &lease() const noexcept { return lease_; }

  void setLease(Variant lease) noexcept { lease_ = std::move(lease); }

  ::orteaf::internal::kernel::core::KernelEntry rebuild() const {
    ::orteaf::internal::kernel::core::KernelEntry entry;
    std::visit(
        [&](const auto &lease_value) {
          using LeaseT = std::decay_t<decltype(lease_value)>;
          if constexpr (detail::KernelMetadataPayloadRebuildable<LeaseT>) {
            if (!lease_value) {
              return;
            }
            auto *payload = lease_value.operator->();
            if (!payload) {
              return;
            }
            payload->rebuildKernelEntry(entry);
          }
        },
        lease_);
    return entry;
  }

  static KernelMetadataLease fromEntry(
      const ::orteaf::internal::kernel::core::KernelEntry &entry) {
    KernelMetadataLease metadata;
    std::visit(
        [&](const auto &lease_value) {
          using LeaseT = std::decay_t<decltype(lease_value)>;
          if constexpr (detail::KernelMetadataFromEntryBuildable<LeaseT>) {
            if (!lease_value) {
              return;
            }
            auto *base_ptr = lease_value.operator->();
            if (!base_ptr) {
              return;
            }
            metadata = ::orteaf::internal::execution::mps::resource::
                MpsKernelMetadata::buildMetadataLeaseFromBase(*base_ptr,
                                                             entry.execute());
          }
        },
        entry.base());
    return metadata;
  }

private:
  Variant lease_{};
};

} // namespace orteaf::internal::kernel::core
