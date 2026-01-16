#pragma once

/**
 * @file dense_tensor_impl_manager.h
 * @brief Manager for DenseTensorImpl instances with pool management.
 *
 * Provides allocation, view operations (transpose, slice, reshape, etc.),
 * and lifetime management for DenseTensorImpl using the PoolManager pattern.
 */

#include <cstddef>
#include <span>
#include <utility>
#include <variant>

#include <orteaf/extension/tensor/dense_tensor_impl.h>
#include <orteaf/extension/tensor/layout/dense_tensor_layout.h>
#include <orteaf/internal/base/handle.h>
#include <orteaf/internal/base/lease/control_block/strong.h>
#include <orteaf/internal/base/manager/pool_manager.h>
#include <orteaf/internal/base/pool/slot_pool.h>
#include <orteaf/internal/dtype/dtype.h>
#include <orteaf/internal/execution/execution.h>
#include <orteaf/internal/storage/manager/storage_manager.h>

namespace orteaf::extension::tensor {

// Forward declarations
class DenseTensorImplManager;

/// @brief Tag for DenseTensorImpl handles.
struct DenseTensorImplTag {};

/// @brief Handle for DenseTensorImpl in the pool.
using DenseTensorImplHandle =
    ::orteaf::internal::base::Handle<DenseTensorImplTag, uint32_t, uint32_t>;

namespace detail {

/// @brief Request for creating a new DenseTensorImpl.
struct DenseTensorImplRequest {
  using Dims = DenseTensorLayout::Dims;
  using DType = ::orteaf::internal::DType;
  using Execution = ::orteaf::internal::execution::Execution;

  Dims shape{};
  DType dtype{DType::F32};
  Execution execution{Execution::Cpu};
  std::size_t alignment{0};
};

/// @brief Request for creating a view (shares storage with source).
struct DenseTensorImplViewRequest {
  DenseTensorLayout layout{};
  ::orteaf::internal::storage::manager::StorageManager::StorageLease storage{};
};

/// @brief Combined request type.
using DenseTensorImplRequestVariant =
    std::variant<DenseTensorImplRequest, DenseTensorImplViewRequest>;

/// @brief Pool traits for DenseTensorImpl.
struct DenseTensorImplPoolTraits {
  using Payload = DenseTensorImpl;
  using Handle = DenseTensorImplHandle;
  using Request = DenseTensorImplRequestVariant;

  struct Context {
    ::orteaf::internal::storage::manager::StorageManager *storage_manager{
        nullptr};
  };

  static constexpr bool destroy_on_release = true;
  static constexpr const char *ManagerName = "DenseTensorImpl manager";

  static void validateRequestOrThrow(const Request &request);
  static bool create(Payload &payload, const Request &request,
                     const Context &context);
  static void destroy(Payload &payload, const Request &request,
                      const Context &context);
};

} // namespace detail

/**
 * @brief Manager for DenseTensorImpl instances.
 *
 * Provides:
 * - Allocation of new tensor impls via create()
 * - View operations that share storage: transpose, slice, reshape, etc.
 * - Pool-based memory management for DenseTensorImpl objects
 */
class DenseTensorImplManager {
public:
  using PayloadPool = ::orteaf::internal::base::pool::SlotPool<
      detail::DenseTensorImplPoolTraits>;
  using ControlBlock = ::orteaf::internal::base::StrongControlBlock<
      DenseTensorImplHandle, DenseTensorImpl, PayloadPool>;

  struct Traits {
    using PayloadPool = DenseTensorImplManager::PayloadPool;
    using ControlBlock = DenseTensorImplManager::ControlBlock;
    struct ControlBlockTag {};
    using PayloadHandle = DenseTensorImplHandle;
    static constexpr const char *Name =
        detail::DenseTensorImplPoolTraits::ManagerName;
  };

  using Core = ::orteaf::internal::base::PoolManager<Traits>;
  using TensorImplLease = Core::StrongLeaseType;
  using Request = detail::DenseTensorImplRequest;
  using ViewRequest = detail::DenseTensorImplViewRequest;
  using Context = detail::DenseTensorImplPoolTraits::Context;
  using Layout = DenseTensorLayout;
  using Dims = Layout::Dims;
  using Dim = Layout::Dim;
  using StorageManager = ::orteaf::internal::storage::manager::StorageManager;

  struct Config {
    std::size_t control_block_capacity{0};
    std::size_t control_block_block_size{0};
    std::size_t control_block_growth_chunk_size{1};
    std::size_t payload_capacity{0};
    std::size_t payload_block_size{0};
    std::size_t payload_growth_chunk_size{1};
  };

  DenseTensorImplManager() = default;
  DenseTensorImplManager(const DenseTensorImplManager &) = delete;
  DenseTensorImplManager &operator=(const DenseTensorImplManager &) = delete;
  DenseTensorImplManager(DenseTensorImplManager &&) = default;
  DenseTensorImplManager &operator=(DenseTensorImplManager &&) = default;
  ~DenseTensorImplManager() = default;

  /**
   * @brief Configure the manager.
   * @param config Pool configuration.
   * @param storage_manager Reference to the storage manager.
   */
  void configure(const Config &config, StorageManager &storage_manager);

  /// @brief Shutdown and release all resources.
  void shutdown();

  /// @brief Check if configured.
  bool isConfigured() const noexcept;

  // ===== Creation =====

  /**
   * @brief Create a new tensor impl with contiguous layout.
   *
   * @param shape The tensor shape.
   * @param dtype The data type.
   * @param execution The execution backend.
   * @param alignment Optional alignment for storage.
   * @return Lease to the created tensor impl.
   */
  TensorImplLease create(std::span<const Dim> shape,
                         ::orteaf::internal::DType dtype,
                         ::orteaf::internal::execution::Execution execution,
                         std::size_t alignment = 0);

  // ===== View Operations (share storage) =====

  /**
   * @brief Create a transposed view.
   * @param src Source tensor impl lease.
   * @param perm Permutation of dimensions.
   * @return Lease to the transposed view.
   */
  TensorImplLease transpose(const TensorImplLease &src,
                            std::span<const std::size_t> perm);

  /**
   * @brief Create a sliced view.
   * @param src Source tensor impl lease.
   * @param starts Start indices.
   * @param sizes Slice sizes.
   * @return Lease to the sliced view.
   */
  TensorImplLease slice(const TensorImplLease &src, std::span<const Dim> starts,
                        std::span<const Dim> sizes);

  /**
   * @brief Create a reshaped view (requires contiguous source).
   * @param src Source tensor impl lease.
   * @param new_shape New shape.
   * @return Lease to the reshaped view.
   */
  TensorImplLease reshape(const TensorImplLease &src,
                          std::span<const Dim> new_shape);

  /**
   * @brief Create a squeezed view (remove size-1 dimensions).
   * @param src Source tensor impl lease.
   * @return Lease to the squeezed view.
   */
  TensorImplLease squeeze(const TensorImplLease &src);

  /**
   * @brief Create an unsqueezed view (add size-1 dimension).
   * @param src Source tensor impl lease.
   * @param dim Dimension to insert.
   * @return Lease to the unsqueezed view.
   */
  TensorImplLease unsqueeze(const TensorImplLease &src, std::size_t dim);

private:
  /// @brief Create a view with the given layout sharing source's storage.
  TensorImplLease createView(Layout layout,
                             StorageManager::StorageLease storage);

  Core core_{};
  StorageManager *storage_manager_{nullptr};
};

} // namespace orteaf::extension::tensor
