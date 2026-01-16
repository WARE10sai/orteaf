#pragma once

/**
 * @file tensor_impl_types.h
 * @brief Registration of TensorImpl types.
 *
 * This file is where contributors register new TensorImpl types.
 *
 * @par How to add a new TensorImpl
 *
 * 1. Create your TensorImpl and Manager classes in extension/tensor/
 *
 * 2. Include your header below and add a TensorImplTraits specialization:
 *    @code
 *    #include <orteaf/extension/tensor/coo_tensor_impl.h>
 *    #include <orteaf/extension/tensor/manager/coo_tensor_impl_manager.h>
 *
 *    template <>
 *    struct TensorImplTraits<CooTensorImpl> {
 *      using Impl = CooTensorImpl;
 *      using Manager = CooTensorImplManager;
 *      static constexpr const char* name = "coo";
 *    };
 *    @endcode
 *
 * 3. Add your impl to RegisteredImpls:
 *    @code
 *    using RegisteredImpls = TensorImplRegistry<
 *      DenseTensorImpl,
 *      CooTensorImpl  // <- Add here
 *    >;
 *    @endcode
 *
 * 4. Add an accessor to TensorApi (internal/tensor/api/tensor_api.h):
 *    @code
 *    static CooTensorImplManager& coo();
 *    @endcode
 */

#include <orteaf/extension/tensor/dense_tensor_impl.h>
#include <orteaf/extension/tensor/manager/dense_tensor_impl_manager.h>
#include <orteaf/internal/tensor/registry/tensor_impl_registry.h>

namespace orteaf::internal::tensor::registry {

// =============================================================================
// TensorImpl Traits Specializations
// =============================================================================

// -----------------------------------------------------------------------------
// DenseTensorImpl (built-in)
// -----------------------------------------------------------------------------

template <>
struct TensorImplTraits<::orteaf::extension::tensor::DenseTensorImpl> {
  using Impl = ::orteaf::extension::tensor::DenseTensorImpl;
  using Manager = ::orteaf::extension::tensor::DenseTensorImplManager;
  static constexpr const char *name = "dense";
};

// -----------------------------------------------------------------------------
// Contributor TensorImpl Traits - Add your specializations below
// -----------------------------------------------------------------------------

// Example for COO sparse tensor:
// #include <orteaf/extension/tensor/coo_tensor_impl.h>
// #include <orteaf/extension/tensor/manager/coo_tensor_impl_manager.h>
//
// template <>
// struct TensorImplTraits<::orteaf::extension::tensor::CooTensorImpl> {
//   using Impl = ::orteaf::extension::tensor::CooTensorImpl;
//   using Manager = ::orteaf::extension::tensor::CooTensorImplManager;
//   static constexpr const char* name = "coo";
// };

// =============================================================================
// Registered TensorImpl Types
// =============================================================================

/**
 * @brief List of all registered TensorImpl types.
 *
 * Contributors: Add your TensorImpl here after defining its traits above.
 */
using RegisteredImpls =
    TensorImplRegistry<::orteaf::extension::tensor::DenseTensorImpl
                       // Contributors: Add new impls here (comma-separated)
                       // , ::orteaf::extension::tensor::CooTensorImpl
                       // , ::orteaf::extension::tensor::CsrTensorImpl
                       >;

} // namespace orteaf::internal::tensor::registry

// Re-export for convenience
namespace orteaf::extension::tensor::registry {
using RegisteredImpls = ::orteaf::internal::tensor::registry::RegisteredImpls;
} // namespace orteaf::extension::tensor::registry
