#include "orteaf/user/tensor/tensor_api.h"

#include "orteaf/internal/diagnostics/error/error.h"

namespace orteaf::user::tensor {

namespace {

bool g_configured = false;

TensorApi::StorageManager &storageManagerSingleton() {
  static TensorApi::StorageManager instance;
  return instance;
}

TensorApi::TensorImplManager &tensorImplManagerSingleton() {
  static TensorApi::TensorImplManager instance;
  return instance;
}

} // namespace

void TensorApi::configure(const Config &config) {
  if (g_configured) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "TensorApi is already configured");
  }

  storageManagerSingleton().configure(config.storage_config);
  tensorImplManagerSingleton().configure(config.tensor_impl_config,
                                         storageManagerSingleton());
  g_configured = true;
}

void TensorApi::shutdown() {
  if (!g_configured) {
    return;
  }
  tensorImplManagerSingleton().shutdown();
  storageManagerSingleton().shutdown();
  g_configured = false;
}

bool TensorApi::isConfigured() noexcept { return g_configured; }

TensorApi::StorageManager &TensorApi::storageManager() {
  if (!g_configured) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "TensorApi is not configured");
  }
  return storageManagerSingleton();
}

TensorApi::TensorImplManager &TensorApi::tensorImplManager() {
  if (!g_configured) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "TensorApi is not configured");
  }
  return tensorImplManagerSingleton();
}

TensorApi::TensorImplLease TensorApi::create(std::span<const Dim> shape,
                                             DType dtype, Execution execution,
                                             std::size_t alignment) {
  return tensorImplManager().create(shape, dtype, execution, alignment);
}

TensorApi::TensorImplLease
TensorApi::transpose(const TensorImplLease &src,
                     std::span<const std::size_t> perm) {
  return tensorImplManager().transpose(src, perm);
}

TensorApi::TensorImplLease TensorApi::slice(const TensorImplLease &src,
                                            std::span<const Dim> starts,
                                            std::span<const Dim> sizes) {
  return tensorImplManager().slice(src, starts, sizes);
}

TensorApi::TensorImplLease TensorApi::reshape(const TensorImplLease &src,
                                              std::span<const Dim> new_shape) {
  return tensorImplManager().reshape(src, new_shape);
}

TensorApi::TensorImplLease TensorApi::squeeze(const TensorImplLease &src) {
  return tensorImplManager().squeeze(src);
}

TensorApi::TensorImplLease TensorApi::unsqueeze(const TensorImplLease &src,
                                                std::size_t dim) {
  return tensorImplManager().unsqueeze(src, dim);
}

} // namespace orteaf::user::tensor
