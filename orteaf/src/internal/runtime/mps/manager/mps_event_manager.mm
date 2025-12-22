#include "orteaf/internal/runtime/mps/manager/mps_event_manager.h"

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/diagnostics/error/error.h"

namespace orteaf::internal::runtime::mps::manager {

void MpsEventManager::initialize(DeviceType device, SlowOps *ops,
                                 std::size_t capacity) {
  shutdown();
  if (device == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS event manager requires a valid device");
  }
  if (ops == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS event manager requires valid ops");
  }
  if (capacity > static_cast<std::size_t>(EventHandle::invalid_index())) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS event manager capacity exceeds maximum handle range");
  }
  device_ = device;
  ops_ = ops;

  payload_pool_.initialize(EventPayloadPoolTraits::Config{capacity});
  const EventControlBlockPoolTraits::Request cb_request{};
  const EventControlBlockPoolTraits::Context cb_context{};
  if (!control_block_pool_.initializeAndCreate(
          EventControlBlockPoolTraits::Config{capacity}, cb_request,
          cb_context)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS event manager failed to initialize control blocks");
  }
  initialized_ = true;
}

void MpsEventManager::shutdown() {
  if (!initialized_) {
    return;
  }
  // Check canShutdown on all created control blocks
  for (std::size_t idx = 0; idx < control_block_pool_.capacity(); ++idx) {
    const ControlBlockHandle handle{static_cast<std::uint32_t>(idx)};
    if (control_block_pool_.isCreated(handle)) {
      const auto *cb = control_block_pool_.get(handle);
      if (cb != nullptr && !cb->canShutdown()) {
        ::orteaf::internal::diagnostics::error::throwError(
            ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
            "MPS event manager shutdown aborted due to active leases");
      }
    }
  }

  const EventPayloadPoolTraits::Request payload_request{};
  const auto payload_context = makePayloadContext();
  payload_pool_.shutdown(payload_request, payload_context);
  control_block_pool_.shutdown();

  device_ = nullptr;
  ops_ = nullptr;
  initialized_ = false;
}

MpsEventManager::EventLease MpsEventManager::acquire() {
  ensureInitialized();
  const EventPayloadPoolTraits::Request request{};
  const auto context = makePayloadContext();
  auto payload_ref = payload_pool_.tryAcquire(request, context);
  if (!payload_ref.valid()) {
    auto reserved = payload_pool_.tryReserve(request, context);
    if (!reserved.valid()) {
      const auto desired = payload_pool_.capacity() + growth_chunk_size_;
      growPools(desired);
      reserved = payload_pool_.tryReserve(request, context);
    }
    if (!reserved.valid()) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
          "MPS event manager has no available slots");
    }
    EventPayloadPoolTraits::Request create_request{reserved.handle};
    if (!payload_pool_.emplace(reserved.handle, create_request, context)) {
      payload_pool_.release(reserved.handle);
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
          "Failed to create MPS event");
    }
    payload_ref = reserved;
  }
  auto cb_handle = ControlBlockHandle::invalid();
  const EventControlBlockPoolTraits::Request cb_request{};
  const EventControlBlockPoolTraits::Context cb_context{};
  auto cb_ref = control_block_pool_.tryAcquire(cb_request, cb_context);
  if (!cb_ref.valid()) {
    const auto desired = control_block_pool_.capacity() + growth_chunk_size_;
    control_block_pool_.growAndCreate(
        EventControlBlockPoolTraits::Config{desired}, cb_request, cb_context);
    cb_ref = control_block_pool_.tryAcquire(cb_request, cb_context);
  }
  if (!cb_ref.valid()) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::OutOfRange,
        "MPS event manager has no available control blocks");
  }
  cb_handle = cb_ref.handle;
  auto *cb = cb_ref.payload_ptr;
  if (cb == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS event control block is unavailable");
  }
  return buildLease(*cb, payload_ref.handle, cb_handle);
}

bool MpsEventManager::isAlive(EventHandle handle) const noexcept {
  return initialized_ && payload_pool_.isValid(handle) &&
         payload_pool_.isCreated(handle);
}

EventPayloadPoolTraits::Context
MpsEventManager::makePayloadContext() const noexcept {
  return EventPayloadPoolTraits::Context{device_, ops_};
}

void MpsEventManager::ensureInitialized() const {
  if (!initialized_) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS event manager has not been initialized");
  }
}

void MpsEventManager::growPools(std::size_t desired_capacity) {
  if (desired_capacity <= payload_pool_.capacity()) {
    return;
  }
  payload_pool_.grow(EventPayloadPoolTraits::Config{desired_capacity});
  const EventControlBlockPoolTraits::Request cb_request{};
  const EventControlBlockPoolTraits::Context cb_context{};
  control_block_pool_.growAndCreate(
      EventControlBlockPoolTraits::Config{desired_capacity}, cb_request,
      cb_context);
}

MpsEventManager::EventLease
MpsEventManager::buildLease(ControlBlock &cb, EventHandle payload_handle,
                            ControlBlockHandle cb_handle) {
  auto *payload_ptr = payload_pool_.get(payload_handle);
  if (payload_ptr == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS event payload is unavailable");
  }
  if (cb.hasPayload()) {
    if (cb.payloadHandle() != payload_handle) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
          "MPS event control block payload mismatch");
    }
  } else if (!cb.tryBindPayload(payload_handle, payload_ptr, &payload_pool_)) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
        "MPS event control block binding failed");
  }
  return EventLease{&cb, &control_block_pool_, cb_handle};
}

} // namespace orteaf::internal::runtime::mps::manager

#endif // ORTEAF_ENABLE_MPS
