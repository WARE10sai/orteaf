#pragma once

/**
 * @file typed_storage_manager.h
 * @brief Generic template for Storage management.
 *
 * This template provides automatic pool management for any Storage type
 * that satisfies the StorageConcept. Similar to TensorImplManager,
 * this allows auto-generation of managers for different storage backends.
 */

#include <cstddef>
#include <utility>

#include <orteaf/internal/base/handle.h>
#include <orteaf/internal/base/lease/control_block/strong.h>
#include <orteaf/internal/base/manager/pool_manager.h>
#include <orteaf/internal/base/pool/slot_pool.h>
#include <orteaf/internal/diagnostics/error/error_macros.h>
#include <orteaf/internal/dtype/dtype.h>
#include <orteaf/internal/execution/execution.h>
#include <orteaf/internal/storage/concepts/storage_concepts.h>

namespace orteaf::internal::storage::manager {

// =============================================================================
// Handle for Storage
// =============================================================================

template <typename Storage> struct StorageTag {};

template <typename Storage>
using StorageHandle =
    ::orteaf::internal::base::Handle<StorageTag<Storage>, uint32_t, uint32_t>;

// =============================================================================
// Pool Traits for Storage
// =============================================================================

namespace detail {

/// @brief Request for creating a new Storage
template <typename Storage> struct TypedStorageRequest {
  using DeviceHandle = typename Storage::DeviceHandle;
  using Layout = typename Storage::Layout;
  using DType = typename Storage::DType;

  DeviceHandle device{DeviceHandle::invalid()};
  DType dtype{DType::F32};
  std::size_t numel{0};
  std::size_t alignment{0};
  Layout layout{};
};

/// @brief Context for pool operations
template <typename Storage> struct TypedStorageContext {};

/// @brief Pool traits for generic Storage
template <typename Storage> struct TypedStoragePoolTraits {
  using Payload = Storage;
  using Handle = StorageHandle<Storage>;
  using Request = TypedStorageRequest<Storage>;
  using Context = TypedStorageContext<Storage>;

  static constexpr bool destroy_on_release = true;
  static constexpr const char *ManagerName = "TypedStorage manager";

  static void validateRequestOrThrow(const Request &request) {
    if (request.numel == 0) {
      ORTEAF_THROW(InvalidArgument, "Storage request requires non-zero numel");
    }
  }

  static bool create(Payload &payload, const Request &request,
                     const Context & /*context*/) {
    // Creation is delegated to Storage::Builder pattern
    // This is a placeholder - actual creation should use the Storage's builder
    return false;
  }

  static void destroy(Payload &payload, const Request & /*request*/,
                      const Context & /*context*/) {
    payload = Payload{};
  }
};

} // namespace detail

// =============================================================================
// Generic TypedStorageManager
// =============================================================================

/**
 * @brief Generic manager for Storage types.
 *
 * Provides automatic pool management for any Storage type.
 *
 * @tparam Storage The Storage type (must satisfy StorageConcept)
 */
template <typename Storage>
  requires concepts::StorageConcept<Storage>
class TypedStorageManager {
public:
  using PayloadPool = ::orteaf::internal::base::pool::SlotPool<
      detail::TypedStoragePoolTraits<Storage>>;
  using ControlBlock =
      ::orteaf::internal::base::StrongControlBlock<StorageHandle<Storage>,
                                                   Storage, PayloadPool>;

  struct Traits {
    using PayloadPool = TypedStorageManager::PayloadPool;
    using ControlBlock = TypedStorageManager::ControlBlock;
    struct ControlBlockTag {};
    using PayloadHandle = StorageHandle<Storage>;
    static constexpr const char *Name =
        detail::TypedStoragePoolTraits<Storage>::ManagerName;
  };

  using Core = ::orteaf::internal::base::PoolManager<Traits>;
  using StorageLease = typename Core::StrongLeaseType;
  using Layout = typename Storage::Layout;
  using DType = typename Storage::DType;
  using Request = detail::TypedStorageRequest<Storage>;
  using Execution = ::orteaf::internal::execution::Execution;

  static constexpr Execution kExecution = Storage::kExecution;

  struct Config {
    std::size_t control_block_capacity{64};
    std::size_t control_block_block_size{16};
    std::size_t control_block_growth_chunk_size{1};
    std::size_t payload_capacity{64};
    std::size_t payload_block_size{16};
    std::size_t payload_growth_chunk_size{1};
  };

  TypedStorageManager() = default;
  TypedStorageManager(const TypedStorageManager &) = delete;
  TypedStorageManager &operator=(const TypedStorageManager &) = delete;
  TypedStorageManager(TypedStorageManager &&) = default;
  TypedStorageManager &operator=(TypedStorageManager &&) = default;
  ~TypedStorageManager() = default;

  void configure(const Config &config);
  StorageLease acquire(const Request &request);
  void shutdown();
  bool isConfigured() const noexcept;

private:
  Core core_{};
};

} // namespace orteaf::internal::storage::manager
