#include "orteaf/internal/runtime/mps/manager/mps_graph_manager.h"

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/diagnostics/error/error.h"

namespace orteaf::internal::runtime::mps::manager {

void MpsGraphManager::initialize(DeviceType device, SlowOps *ops,
                                 std::size_t capacity) {
  shutdown();
  if (device == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS graph manager requires a valid device");
  }
  if (ops == nullptr) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS graph manager requires valid ops");
  }
  if (capacity > static_cast<std::size_t>(GraphHandle::invalid_index())) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS graph manager capacity exceeds maximum handle range");
  }
  device_ = device;
  ops_ = ops;
  clearCacheStates();
  key_to_index_.clear();
  growth_chunk_size_ = capacity > 0 ? capacity : 1;
  if (capacity > 0) {
    states_.reserve(capacity);
  }
  initialized_ = true;
}

void MpsGraphManager::shutdown() {
  if (!initialized_) {
    return;
  }
  for (std::size_t i = 0; i < states_.size(); ++i) {
    State &state = states_[i];
    if (state.alive) {
      destroyResource(state.resource);
      state.alive = false;
    }
  }
  clearCacheStates();
  key_to_index_.clear();
  device_ = nullptr;
  ops_ = nullptr;
  initialized_ = false;
}

MpsGraphManager::GraphLease
MpsGraphManager::acquire(const GraphKey &key, const CompileFn &compile_fn) {
  ensureInitialized();
  validateKey(key);
  if (!compile_fn) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "MPS graph compile function cannot be empty");
  }

  // Check if already cached
  if (auto it = key_to_index_.find(key); it != key_to_index_.end()) {
    incrementUseCount(it->second);
    return GraphLease{this, createHandle<GraphHandle>(it->second),
                      states_[it->second].resource.executable};
  }

  // Create new entry
  const std::size_t index = allocateSlot();
  State &state = states_[index];

  try {
    state.resource.graph = ops_->createGraph();
    state.resource.executable = compile_fn(state.resource.graph, device_, ops_);
    if (state.resource.executable == nullptr) {
      ::orteaf::internal::diagnostics::error::throwError(
          ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidState,
          "MPS graph compile function returned null executable");
    }
    markSlotAlive(index);
    key_to_index_.emplace(key, index);
    return GraphLease{this, createHandle<GraphHandle>(index),
                      state.resource.executable};
  } catch (...) {
    destroyResource(state.resource);
    throw;
  }
}

void MpsGraphManager::release(GraphLease &lease) noexcept {
  if (!lease) {
    return;
  }
  decrementUseCount(static_cast<std::size_t>(lease.handle().index));
  lease.invalidate();
}

void MpsGraphManager::validateKey(const GraphKey &key) const {
  if (key.identifier.empty()) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "Graph identifier cannot be empty");
  }
  if (key.data_type == ::orteaf::internal::runtime::mps::platform::wrapper::
                           MpsGraphDataType::kInvalid) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "Graph data type must be valid");
  }
  if (key.target_tensor_count == 0) {
    ::orteaf::internal::diagnostics::error::throwError(
        ::orteaf::internal::diagnostics::error::OrteafErrc::InvalidArgument,
        "Graph target tensor count must be > 0");
  }
}

void MpsGraphManager::destroyResource(MpsGraphResource &resource) {
  if (resource.executable != nullptr) {
    ops_->destroyGraphExecutable(resource.executable);
    resource.executable = nullptr;
  }
  if (resource.graph != nullptr) {
    ops_->destroyGraph(resource.graph);
    resource.graph = nullptr;
  }
}

} // namespace orteaf::internal::runtime::mps::manager

#endif // ORTEAF_ENABLE_MPS
