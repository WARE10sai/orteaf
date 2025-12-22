#include "orteaf/internal/runtime/mps/manager/mps_device_manager.h"

#if ORTEAF_ENABLE_MPS

namespace orteaf::internal::runtime::mps::manager {

void MpsDeviceManager::initialize(SlowOps *slow_ops) {
  shutdown();

  if (slow_ops == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS device manager requires valid ops");
  }
  ops_ = slow_ops;

  const int device_count = ops_->getDeviceCount();
  if (device_count <= 0) {
    core_.payloadPool().initialize(DevicePayloadPoolTraits::Config{0});
    core_.initializeControlBlockPool(0);
    core_.setInitialized(true);
    return;
  }

  const auto capacity = static_cast<std::size_t>(device_count);
  const auto payload_context = makePayloadContext();
  const DevicePayloadPoolTraits::Request payload_request{};
  core_.payloadPool().initializeAndCreate(
      DevicePayloadPoolTraits::Config{capacity}, payload_request,
      payload_context);

  core_.initializeControlBlockPool(capacity);
  core_.setInitialized(true);
}

void MpsDeviceManager::shutdown() {
  if (!core_.isInitialized()) {
    return;
  }
  // Check canShutdown on all created control blocks
  core_.checkCanShutdownOrThrow();

  const DevicePayloadPoolTraits::Request payload_request{};
  const auto payload_context = makePayloadContext();
  core_.payloadPool().shutdown(payload_request, payload_context);
  core_.shutdownControlBlockPool();
  ops_ = nullptr;
  core_.setInitialized(false);
}

MpsDeviceManager::DeviceLease MpsDeviceManager::acquire(DeviceHandle handle) {
  core_.ensureInitialized();
  if (!core_.payloadPool().isValid(handle)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS device handle is invalid");
  }
  if (!core_.payloadPool().isCreated(handle)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS device is unavailable");
  }
  auto *payload_ptr = core_.payloadPool().get(handle);
  if (payload_ptr == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS device payload is unavailable");
  }

  auto cb_ref = core_.acquireControlBlock();
  if (!cb_ref.payload_ptr->tryBindPayload(handle, payload_ptr,
                                          &core_.payloadPool())) {
    core_.releaseControlBlock(cb_ref.handle);
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS device control block binding failed");
  }
  return DeviceLease{cb_ref.payload_ptr, &core_.controlBlockPool(),
                     cb_ref.handle};
}

::orteaf::internal::architecture::Architecture
MpsDeviceManager::getArch(DeviceHandle handle) const {
  if (!core_.isInitialized()) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS devices not initialized");
  }
  if (!core_.payloadPool().isValid(handle)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS device handle is invalid");
  }
  if (!core_.payloadPool().isCreated(handle)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS device is unavailable");
  }
  return core_.payloadPool().get(handle)->arch;
}

DevicePayloadPoolTraits::Context
MpsDeviceManager::makePayloadContext() const noexcept {
  return DevicePayloadPoolTraits::Context{
      ops_, command_queue_initial_capacity_, heap_initial_capacity_,
      library_initial_capacity_, graph_initial_capacity_};
}

} // namespace orteaf::internal::runtime::mps::manager

#endif // ORTEAF_ENABLE_MPS
