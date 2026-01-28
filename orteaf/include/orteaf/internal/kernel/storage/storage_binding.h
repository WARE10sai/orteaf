#pragma once

#include <orteaf/internal/kernel/storage/storage_key.h>

namespace orteaf::internal::kernel {

/**
 * @brief Generic storage binding structure for kernel arguments.
 *
 * Represents a bound storage resource with its key and lease.
 * This template can be used with a type-erased StorageLease to avoid
 * backend-specific bindings. Access pattern information is available through
 * the StorageId metadata.
 *
 * @tparam StorageLease The storage lease type (type-erased or backend-specific)
 *
 * Example:
 * @code
 * using AnyStorageBinding = StorageBinding<StorageLease>;
 * @endcode
 */
template <typename StorageLease>
struct StorageBinding {
  /**
   * @brief Storage identifier.
   *
 * Identifies the semantic role of this storage (e.g., Input0, Output) and
 * the tensor-internal role (e.g., Data, Index).
 * Access pattern can be queried via StorageTypeInfo<id>::kAccess.
 */
  StorageKey key;

  /**
   * @brief Backend-specific storage lease.
   *
   * Holds the actual storage resource for the target execution backend.
   */
  StorageLease lease;
};

} // namespace orteaf::internal::kernel
