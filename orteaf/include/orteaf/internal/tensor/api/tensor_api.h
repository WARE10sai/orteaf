#pragma once

/**
 * @file tensor_api.h
 * @brief Internal API for tensor management.
 *
 * TensorApi provides centralized access to StorageManager and
 * tensor impl managers via the TensorImplRegistry.
 * This is internal infrastructure - users should use the Tensor class.
 */

#include <span>

#include <orteaf/extension/tensor/registry/tensor_impl_types.h>
#include <orteaf/internal/storage/manager/storage_manager.h>

namespace orteaf::internal::tensor::api {

/**
 * @brief Internal API for tensor management.
 *
 * Holds StorageManager and the TensorImplRegistry which manages
 * all registered tensor impl managers.
 *
 * Must be configured before use and shutdown when done.
 *
 * @note This is internal infrastructure. Users should use Tensor class.
 *
 * @par Adding a new TensorImpl accessor
 * After registering a new impl in tensor_impl_registry.h,
 * add an accessor method here:
 * @code
 * static CooTensorImplManager& coo() { return registry().get<CooTensorImpl>();
 * }
 * @endcode
 */
class TensorApi {
public:
  using StorageManager = ::orteaf::internal::storage::manager::StorageManager;
  using Registry = ::orteaf::internal::tensor::registry::RegisteredImpls;
  using DenseTensorImpl = ::orteaf::extension::tensor::DenseTensorImpl;
  using DenseTensorImplManager =
      ::orteaf::extension::tensor::DenseTensorImplManager;
  using TensorImplLease = DenseTensorImplManager::TensorImplLease;
  using DType = ::orteaf::internal::DType;
  using Execution = ::orteaf::internal::execution::Execution;
  using Dim = ::orteaf::extension::tensor::DenseTensorLayout::Dim;

  struct Config {
    StorageManager::Config storage_config{};
    Registry::Config registry_config{};
  };

  TensorApi() = delete;

  /// @brief Configure the API with all managers.
  static void configure(const Config &config);

  /// @brief Shutdown all managers.
  static void shutdown();

  /// @brief Check if configured.
  static bool isConfigured() noexcept;

  /// @brief Access the storage manager.
  static StorageManager &storage();

  /// @brief Access the registry.
  static Registry &registry();

  // ===== TensorImpl Manager Accessors =====
  // Contributors: Add accessor for new impls here

  /// @brief Access the dense tensor impl manager.
  static DenseTensorImplManager &dense();

  // Future: Add more accessors
  // static CooTensorImplManager& coo();
  // static CsrTensorImplManager& csr();

  // ===== Convenience methods for DenseTensorImpl =====

  /// @brief Create a new dense tensor impl.
  static TensorImplLease create(std::span<const Dim> shape, DType dtype,
                                Execution execution, std::size_t alignment = 0);

  /// @brief Create a transposed view.
  static TensorImplLease transpose(const TensorImplLease &src,
                                   std::span<const std::size_t> perm);

  /// @brief Create a sliced view.
  static TensorImplLease slice(const TensorImplLease &src,
                               std::span<const Dim> starts,
                               std::span<const Dim> sizes);

  /// @brief Create a reshaped view.
  static TensorImplLease reshape(const TensorImplLease &src,
                                 std::span<const Dim> new_shape);

  /// @brief Create a squeezed view.
  static TensorImplLease squeeze(const TensorImplLease &src);

  /// @brief Create an unsqueezed view.
  static TensorImplLease unsqueeze(const TensorImplLease &src, std::size_t dim);
};

} // namespace orteaf::internal::tensor::api
