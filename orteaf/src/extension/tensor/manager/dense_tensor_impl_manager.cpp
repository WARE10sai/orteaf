#include "orteaf/extension/tensor/manager/dense_tensor_impl_manager.h"

#include <utility>
#include <variant>

#include "orteaf/internal/diagnostics/error/error.h"
#include "orteaf/internal/storage/manager/storage_request.h"

namespace orteaf::extension::tensor {

namespace detail {

void DenseTensorImplPoolTraits::validateRequestOrThrow(const Request &request) {
  std::visit(
      [](const auto &req) {
        using T = std::decay_t<decltype(req)>;
        if constexpr (std::is_same_v<T, DenseTensorImplRequest>) {
          if (req.shape.empty()) {
            // Scalar is allowed (empty shape)
          }
          // Additional validations can be added here
        } else if constexpr (std::is_same_v<T, DenseTensorImplViewRequest>) {
          if (!req.storage) {
            ::orteaf::internal::diagnostics::error::throwError(
                ::orteaf::internal::diagnostics::error::OrteafErrc::
                    InvalidArgument,
                "DenseTensorImplViewRequest requires valid storage");
          }
        }
      },
      request);
}

bool DenseTensorImplPoolTraits::create(Payload &payload, const Request &request,
                                       const Context &context) {
  return std::visit(
      [&](const auto &req) -> bool {
        using T = std::decay_t<decltype(req)>;
        if constexpr (std::is_same_v<T, DenseTensorImplRequest>) {
          // Create new storage
          if (context.storage_manager == nullptr) {
            return false;
          }

          // Calculate numel from shape
          std::int64_t numel = 1;
          for (auto dim : req.shape) {
            if (dim == 0) {
              numel = 0;
              break;
            }
            numel *= dim;
          }

          // Create storage request based on execution backend
          using namespace ::orteaf::internal::storage::manager;
          using Execution = ::orteaf::internal::execution::Execution;

          StorageRequest storage_request;
          if (req.execution == Execution::Cpu) {
            CpuStorageRequest cpu_req{};
            cpu_req.device =
                ::orteaf::internal::execution::cpu::CpuDeviceHandle{0};
            cpu_req.dtype = req.dtype;
            cpu_req.numel = static_cast<std::size_t>(numel);
            cpu_req.alignment = req.alignment;
            storage_request = cpu_req;
          }
#if ORTEAF_ENABLE_MPS
          else if (req.execution == Execution::Mps) {
            MpsStorageRequest mps_req{};
            mps_req.device =
                ::orteaf::internal::execution::mps::MpsDeviceHandle{0};
            mps_req.dtype = req.dtype;
            mps_req.numel = static_cast<std::size_t>(numel);
            mps_req.alignment = req.alignment;
            // TODO: Configure heap_key appropriately
            storage_request = mps_req;
          }
#endif
          else {
            return false;
          }

          auto storage_lease =
              context.storage_manager->acquire(storage_request);
          auto layout = DenseTensorLayout::contiguous(req.shape);
          payload =
              DenseTensorImpl(std::move(layout), std::move(storage_lease));
          return true;

        } else if constexpr (std::is_same_v<T, DenseTensorImplViewRequest>) {
          // Create view (share storage)
          payload = DenseTensorImpl(req.layout, req.storage);
          return true;
        } else {
          return false;
        }
      },
      request);
}

void DenseTensorImplPoolTraits::destroy(Payload &payload, const Request &,
                                        const Context &) {
  payload = Payload{};
}

} // namespace detail

void DenseTensorImplManager::configure(const Config &config,
                                       StorageManager &storage_manager) {
  storage_manager_ = &storage_manager;

  std::size_t payload_capacity = config.payload_capacity;
  if (payload_capacity == 0) {
    payload_capacity = 64;
  }
  std::size_t payload_block_size = config.payload_block_size;
  if (payload_block_size == 0) {
    payload_block_size = 16;
  }
  std::size_t control_block_capacity = config.control_block_capacity;
  if (control_block_capacity == 0) {
    control_block_capacity = 64;
  }
  std::size_t control_block_block_size = config.control_block_block_size;
  if (control_block_block_size == 0) {
    control_block_block_size = 16;
  }

  detail::DenseTensorImplRequestVariant request{};
  Context context{storage_manager_};

  Core::Builder<detail::DenseTensorImplRequestVariant, Context> builder{};
  builder.withControlBlockCapacity(control_block_capacity)
      .withControlBlockBlockSize(control_block_block_size)
      .withControlBlockGrowthChunkSize(config.control_block_growth_chunk_size)
      .withPayloadCapacity(payload_capacity)
      .withPayloadBlockSize(payload_block_size)
      .withPayloadGrowthChunkSize(config.payload_growth_chunk_size)
      .withRequest(request)
      .withContext(context)
      .configure(core_);
}

void DenseTensorImplManager::shutdown() {
  detail::DenseTensorImplRequestVariant request{};
  Context context{storage_manager_};
  core_.shutdown(request, context);
  storage_manager_ = nullptr;
}

bool DenseTensorImplManager::isConfigured() const noexcept {
  return core_.isConfigured();
}

DenseTensorImplManager::TensorImplLease DenseTensorImplManager::create(
    std::span<const Dim> shape, ::orteaf::internal::DType dtype,
    ::orteaf::internal::execution::Execution execution, std::size_t alignment) {
  core_.ensureConfigured();

  detail::DenseTensorImplRequest req{};
  req.shape.assign(shape.begin(), shape.end());
  req.dtype = dtype;
  req.execution = execution;
  req.alignment = alignment;

  detail::DenseTensorImplRequestVariant request{req};
  Context context{storage_manager_};

  auto payload_handle = core_.reserveUncreatedPayloadOrGrow();
  if (!payload_handle.isValid()) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
        "DenseTensorImplManager has no available slots");
  }

  if (!core_.emplacePayload(payload_handle, request, context)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "DenseTensorImplManager failed to create tensor impl");
  }

  return core_.acquireStrongLease(payload_handle);
}

DenseTensorImplManager::TensorImplLease
DenseTensorImplManager::createView(Layout layout,
                                   StorageManager::StorageLease storage) {
  core_.ensureConfigured();

  detail::DenseTensorImplViewRequest req{};
  req.layout = std::move(layout);
  req.storage = std::move(storage);

  detail::DenseTensorImplRequestVariant request{req};
  Context context{storage_manager_};

  auto payload_handle = core_.reserveUncreatedPayloadOrGrow();
  if (!payload_handle.isValid()) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
        "DenseTensorImplManager has no available slots");
  }

  if (!core_.emplacePayload(payload_handle, request, context)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "DenseTensorImplManager failed to create view");
  }

  return core_.acquireStrongLease(payload_handle);
}

DenseTensorImplManager::TensorImplLease
DenseTensorImplManager::transpose(const TensorImplLease &src,
                                  std::span<const std::size_t> perm) {
  auto new_layout = src->layout().transpose(perm);
  return createView(std::move(new_layout), src->storageLease());
}

DenseTensorImplManager::TensorImplLease
DenseTensorImplManager::slice(const TensorImplLease &src,
                              std::span<const Dim> starts,
                              std::span<const Dim> sizes) {
  auto new_layout = src->layout().slice(starts, sizes);
  return createView(std::move(new_layout), src->storageLease());
}

DenseTensorImplManager::TensorImplLease
DenseTensorImplManager::reshape(const TensorImplLease &src,
                                std::span<const Dim> new_shape) {
  auto new_layout = src->layout().reshape(new_shape);
  return createView(std::move(new_layout), src->storageLease());
}

DenseTensorImplManager::TensorImplLease
DenseTensorImplManager::squeeze(const TensorImplLease &src) {
  auto new_layout = src->layout().squeeze();
  return createView(std::move(new_layout), src->storageLease());
}

DenseTensorImplManager::TensorImplLease
DenseTensorImplManager::unsqueeze(const TensorImplLease &src, std::size_t dim) {
  auto new_layout = src->layout().unsqueeze(dim);
  return createView(std::move(new_layout), src->storageLease());
}

} // namespace orteaf::extension::tensor
