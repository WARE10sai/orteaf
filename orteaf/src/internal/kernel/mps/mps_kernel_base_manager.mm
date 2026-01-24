#if ORTEAF_ENABLE_MPS

#include <initializer_list>
#include <utility>

#include "orteaf/internal/kernel/mps/mps_kernel_base_manager.h"

namespace orteaf::internal::kernel::mps {

bool MpsKernelBaseManager::PayloadPoolTraits::create(
    Payload &payload, const Request &request, const Context &) {
  if (request.keys == nullptr || request.key_count == 0) {
    return false;
  }
  // Create KernelBase with the provided keys
  payload = KernelBase{};
  payload.reserveKeys(request.key_count);
  for (std::size_t i = 0; i < request.key_count; ++i) {
    const auto &k = request.keys[i];
    payload.addKey(k.library, k.function);
  }
  return true;
}

void MpsKernelBaseManager::PayloadPoolTraits::destroy(Payload &payload,
                                                     const Request &,
                                                     const Context &) {
  // KernelBase destructor handles cleanup automatically
  payload = KernelBase{};
}

void MpsKernelBaseManager::configure(const Config &config) {
  shutdown();
  const PayloadPoolTraits::Request payload_request{};
  const PayloadPoolTraits::Context payload_context{};
  Core::Builder<PayloadPoolTraits::Request, PayloadPoolTraits::Context>{}
      .withControlBlockCapacity(config.control_block_capacity)
      .withControlBlockBlockSize(config.control_block_block_size)
      .withControlBlockGrowthChunkSize(
          config.control_block_growth_chunk_size)
      .withPayloadCapacity(config.payload_capacity)
      .withPayloadBlockSize(config.payload_block_size)
      .withPayloadGrowthChunkSize(config.payload_growth_chunk_size)
      .withRequest(payload_request)
      .withContext(payload_context)
      .configure(core_);
  configured_ = true;
}

void MpsKernelBaseManager::shutdown() {
  lifetime_.clear();
  key_to_index_.clear();
  const PayloadPoolTraits::Request payload_request{};
  const PayloadPoolTraits::Context payload_context{};
  core_.shutdown(payload_request, payload_context);
  configured_ = false;
}

MpsKernelBaseManager::KernelBaseLease
MpsKernelBaseManager::acquire(const KernelBaseKey &key,
                              KernelBase::KeyLiteral *keys,
                              std::size_t key_count) {
  if (!configured_ || keys == nullptr || key_count == 0) {
    return KernelBaseLease{};
  }

  if (auto it = key_to_index_.find(key); it != key_to_index_.end()) {
    auto handle = KernelBaseHandle{
        static_cast<KernelBaseHandle::index_type>(it->second)};
    if (core_.isAlive(handle)) {
      auto cached = lifetime_.get(handle);
      if (cached) {
        return cached;
      }
      auto lease = core_.acquireStrongLease(handle);
      lifetime_.set(lease);
      return lease;
    }
    key_to_index_.erase(it);
  }

  PayloadPoolTraits::Request request{};
  request.key = key;
  request.keys = keys;
  request.key_count = key_count;
  const PayloadPoolTraits::Context context{};

  auto handle = core_.reserveUncreatedPayloadOrGrow();
  if (!handle.isValid()) {
    return KernelBaseLease{};
  }
  if (!core_.emplacePayload(handle, request, context)) {
    return KernelBaseLease{};
  }

  key_to_index_.emplace(key, static_cast<std::size_t>(handle.index));
  auto lease = core_.acquireStrongLease(handle);
  lifetime_.set(lease);
  return lease;
}

MpsKernelBaseManager::KernelBaseLease
MpsKernelBaseManager::acquire(KernelBaseHandle handle) {
  if (!configured_ || !core_.isAlive(handle)) {
    return KernelBaseLease{};
  }

  auto cached = lifetime_.get(handle);
  if (cached) {
    return cached;
  }

  auto lease = core_.acquireStrongLease(handle);
  lifetime_.set(lease);
  return lease;
}

} // namespace orteaf::internal::kernel::mps

#endif // ORTEAF_ENABLE_MPS
