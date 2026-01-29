#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/execution/mps/manager/mps_kernel_base_manager.h"
#include "orteaf/internal/diagnostics/error/error.h"
#include "orteaf/internal/execution/mps/manager/mps_device_manager.h"

namespace orteaf::internal::execution::mps::manager {

using DeviceLease = ::orteaf::internal::execution::mps::manager::
    MpsDeviceManager::DeviceLease;

// =============================================================================
// PayloadPoolTraits implementation
// =============================================================================

bool KernelBasePayloadPoolTraits::create(Payload &payload,
                                          const Request &request,
                                          const Context &context) {
  if (context.device_lease == nullptr || !(*context.device_lease)) {
    return false;
  }

  // Acquire pipeline leases for each library/function key pair
  payload.reserve(request.keys.size());
  for (const auto &key : request.keys) {
    // Get library lease via DeviceLease->libraryManager()
    auto library_lease =
        (*context.device_lease)->libraryManager().acquire(key.first);
    if (!library_lease) {
      // Failed to acquire library, cleanup and return false
      payload.reset();
      return false;
    }

    // Get library resource from the lease using operator->()
    auto *library_resource = library_lease.operator->();
    if (library_resource == nullptr) {
      payload.reset();
      return false;
    }

    // Acquire pipeline lease from pipeline manager
    auto pipeline_lease =
        library_resource->pipelineManager().acquire(key.second);
    if (!pipeline_lease) {
      payload.reset();
      return false;
    }

    // Store the pipeline lease (library lease will be held by pipeline)
    payload.addPipeline(std::move(pipeline_lease));
  }

  return true;
}

void KernelBasePayloadPoolTraits::destroy(Payload &payload,
                                           const Request &,
                                           const Context &) {
  // Pipeline leases are automatically released on destruction
  payload.reset();
}

// =============================================================================
// MpsKernelBaseManager implementation
// =============================================================================

void MpsKernelBaseManager::configure(const InternalConfig &config) {
  shutdown();
  
  const auto &cfg = config.public_config;
  const KernelBasePayloadPoolTraits::Request payload_request{};
  // Note: payload_context requires device_lease, set during acquire()
  
  Core::Builder<KernelBasePayloadPoolTraits::Request,
                KernelBasePayloadPoolTraits::Context>{}
      .withControlBlockCapacity(cfg.control_block_capacity)
      .withControlBlockBlockSize(cfg.control_block_block_size)
      .withControlBlockGrowthChunkSize(cfg.control_block_growth_chunk_size)
      .withPayloadCapacity(cfg.payload_capacity)
      .withPayloadBlockSize(cfg.payload_block_size)
      .withPayloadGrowthChunkSize(cfg.payload_growth_chunk_size)
      .withRequest(payload_request)
      .withContext({nullptr}) // Context set per-acquire
      .configure(core_);
}

void MpsKernelBaseManager::shutdown() {
  if (!core_.isConfigured()) {
    return;
  }
  
  const KernelBasePayloadPoolTraits::Request payload_request{};
  const KernelBasePayloadPoolTraits::Context payload_context{nullptr};
  core_.shutdown(payload_request, payload_context);
}

MpsKernelBaseManager::KernelBaseLease
MpsKernelBaseManager::acquire(
    const ::orteaf::internal::base::HeapVector<Key> &keys,
    DeviceLease &device_lease) {
  core_.ensureConfigured();
  
  KernelBasePayloadPoolTraits::Request request{};
  request.keys = keys;
  const auto context = makePayloadContext(device_lease);
  
  auto handle = core_.acquirePayloadOrGrowAndCreate(request, context);
  if (!handle.isValid()) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
        "MPS kernel base manager has no available slots");
  }
  
  return core_.acquireStrongLease(handle);
}

KernelBasePayloadPoolTraits::Context
MpsKernelBaseManager::makePayloadContext(
    DeviceLease &device_lease) const noexcept {
  KernelBasePayloadPoolTraits::Context context{};
  context.device_lease = &device_lease;
  return context;
}

} // namespace orteaf::internal::execution::mps::manager

#endif // ORTEAF_ENABLE_MPS
