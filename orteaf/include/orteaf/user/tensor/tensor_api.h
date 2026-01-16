#pragma once

/**
 * @file tensor_api.h
 * @brief Singleton API for tensor operations.
 *
 * TensorApi provides centralized access to StorageManager and
 * DenseTensorImplManager, enabling tensor creation and manipulation.
 */

#include <orteaf/extension/tensor/manager/dense_tensor_impl_manager.h>
#include <orteaf/internal/storage/manager/storage_manager.h>

namespace orteaf::user::tensor {

/**
 * @brief Singleton API for tensor operations.
 *
 * Holds both StorageManager and DenseTensorImplManager.
 * Must be configured before use and shutdown when done.
 *
 * @par Example
 * @code
 * TensorApi::configure({});
 * auto impl = TensorApi::create({3, 4}, DType::F32, Execution::Cpu);
 * TensorApi::shutdown();
 * @endcode
 */
class TensorApi {
public:
  using StorageManager = ::orteaf::internal::storage::manager::StorageManager;
  using TensorImplManager = ::orteaf::extension::tensor::DenseTensorImplManager;
  using TensorImplLease = TensorImplManager::TensorImplLease;
  using DType = ::orteaf::internal::DType;
  using Execution = ::orteaf::internal::execution::Execution;
  using Dim = ::orteaf::extension::tensor::DenseTensorLayout::Dim;

  struct Config {
    StorageManager::Config storage_config{};
    TensorImplManager::Config tensor_impl_config{};
  };

  TensorApi() = delete;

  /// @brief Configure the API with both managers.
  static void configure(const Config &config);

  /// @brief Shutdown both managers.
  static void shutdown();

  /// @brief Check if configured.
  static bool isConfigured() noexcept;

  /// @brief Access the storage manager.
  static StorageManager &storageManager();

  /// @brief Access the tensor impl manager.
  static TensorImplManager &tensorImplManager();

  // ===== Convenience methods =====

  /**
   * @brief Create a new dense tensor impl.
   * @param shape Tensor shape.
   * @param dtype Data type.
   * @param execution Execution backend.
   * @param alignment Optional alignment.
   * @return Lease to the created tensor impl.
   */
  static TensorImplLease create(std::span<const Dim> shape, DType dtype,
                                Execution execution, std::size_t alignment = 0);

  /**
   * @brief Create a transposed view.
   */
  static TensorImplLease transpose(const TensorImplLease &src,
                                   std::span<const std::size_t> perm);

  /**
   * @brief Create a sliced view.
   */
  static TensorImplLease slice(const TensorImplLease &src,
                               std::span<const Dim> starts,
                               std::span<const Dim> sizes);

  /**
   * @brief Create a reshaped view.
   */
  static TensorImplLease reshape(const TensorImplLease &src,
                                 std::span<const Dim> new_shape);

  /**
   * @brief Create a squeezed view.
   */
  static TensorImplLease squeeze(const TensorImplLease &src);

  /**
   * @brief Create an unsqueezed view.
   */
  static TensorImplLease unsqueeze(const TensorImplLease &src, std::size_t dim);

private:
  static StorageManager &storage_manager_instance();
  static TensorImplManager &tensor_impl_manager_instance();
};

} // namespace orteaf::user::tensor
