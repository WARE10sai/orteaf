#pragma once

/**
 * @file tensor.h
 * @brief User-facing Tensor class with type-erased impl.
 *
 * Tensor wraps different tensor implementations (DenseTensorImpl, etc.)
 * via type erasure, providing a unified interface for tensor operations.
 */

#include <span>
#include <variant>

#include <orteaf/extension/tensor/dense_tensor_impl.h>
#include <orteaf/extension/tensor/layout/dense_tensor_layout.h>
#include <orteaf/extension/tensor/manager/dense_tensor_impl_manager.h>
#include <orteaf/internal/dtype/dtype.h>
#include <orteaf/internal/execution/execution.h>

namespace orteaf::user::tensor {

/**
 * @brief Type-erased tensor implementation variant.
 *
 * Can hold leases to different tensor implementations:
 * - DenseTensorImplLease (dense/strided tensors)
 * - (Future: SparseTensorImplLease, QuantizedTensorImplLease, etc.)
 */
using TensorImplVariant = std::variant<
    std::monostate,
    ::orteaf::extension::tensor::DenseTensorImplManager::TensorImplLease
    // Future: SparseTensorImplLease, QuantizedTensorImplLease, etc.
    >;

/**
 * @brief User-facing tensor class.
 *
 * Provides a unified interface for tensor operations regardless of
 * the underlying implementation type. Uses type erasure via variant.
 *
 * @par Example
 * @code
 * auto a = Tensor::dense({3, 4}, DType::F32, Execution::Cpu);
 * auto b = a.transpose({1, 0});
 * auto c = b.reshape({12});
 * @endcode
 */
class Tensor {
public:
  using DenseTensorImplLease =
      ::orteaf::extension::tensor::DenseTensorImplManager::TensorImplLease;
  using Layout = ::orteaf::extension::tensor::DenseTensorLayout;
  using Dims = Layout::Dims;
  using Dim = Layout::Dim;
  using DType = ::orteaf::internal::DType;
  using Execution = ::orteaf::internal::execution::Execution;

  /**
   * @brief Default constructor. Creates an invalid tensor.
   */
  Tensor() = default;

  /**
   * @brief Construct from a dense tensor impl lease.
   */
  explicit Tensor(DenseTensorImplLease impl) : impl_(std::move(impl)) {}

  Tensor(const Tensor &) = default;
  Tensor &operator=(const Tensor &) = default;
  Tensor(Tensor &&) = default;
  Tensor &operator=(Tensor &&) = default;
  ~Tensor() = default;

  // ===== Factory methods =====

  /**
   * @brief Create a dense tensor.
   * @param shape Tensor shape.
   * @param dtype Data type.
   * @param execution Execution backend.
   * @param alignment Optional alignment.
   * @return Created tensor.
   */
  static Tensor dense(std::span<const Dim> shape, DType dtype,
                      Execution execution, std::size_t alignment = 0);

  // ===== Type queries =====

  /// @brief Check if this tensor is valid (has an impl).
  bool valid() const noexcept;

  /// @brief Check if this tensor is a dense tensor.
  bool isDense() const noexcept;

  // ===== Accessors =====

  /// @brief Return the data type.
  DType dtype() const;

  /// @brief Return the execution backend.
  Execution execution() const;

  /// @brief Return the tensor shape.
  Dims shape() const;

  /// @brief Return the tensor strides.
  Dims strides() const;

  /// @brief Return the number of elements.
  Dim numel() const;

  /// @brief Return the rank (number of dimensions).
  std::size_t rank() const;

  /// @brief Check if the tensor is contiguous.
  bool isContiguous() const;

  // ===== View operations =====

  /**
   * @brief Create a transposed view.
   * @param perm Permutation of dimensions.
   * @return Transposed tensor.
   */
  Tensor transpose(std::span<const std::size_t> perm) const;

  /**
   * @brief Create a sliced view.
   * @param starts Start indices.
   * @param sizes Slice sizes.
   * @return Sliced tensor.
   */
  Tensor slice(std::span<const Dim> starts, std::span<const Dim> sizes) const;

  /**
   * @brief Create a reshaped view.
   * @param new_shape New shape.
   * @return Reshaped tensor.
   */
  Tensor reshape(std::span<const Dim> new_shape) const;

  /**
   * @brief Create a squeezed view (remove size-1 dimensions).
   * @return Squeezed tensor.
   */
  Tensor squeeze() const;

  /**
   * @brief Create an unsqueezed view (add size-1 dimension).
   * @param dim Dimension to insert.
   * @return Unsqueezed tensor.
   */
  Tensor unsqueeze(std::size_t dim) const;

  // ===== Access to underlying impl =====

  /// @brief Get the underlying variant.
  const TensorImplVariant &implVariant() const noexcept { return impl_; }

  /// @brief Try to get as dense impl lease.
  const DenseTensorImplLease *tryAsDense() const noexcept;

private:
  TensorImplVariant impl_{};
};

} // namespace orteaf::user::tensor
