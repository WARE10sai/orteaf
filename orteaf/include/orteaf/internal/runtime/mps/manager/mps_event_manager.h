#pragma once

#if ORTEAF_ENABLE_MPS

#include <cstddef>
#include <cstdint>

#include "orteaf/internal/base/handle.h"
#include "orteaf/internal/runtime/base/lease/control_block/shared.h"
#include "orteaf/internal/runtime/base/lease/strong_lease.h"
#include "orteaf/internal/runtime/base/pool/fixed_slot_store.h"
#include "orteaf/internal/runtime/base/pool/slot_pool.h"
#include "orteaf/internal/runtime/mps/platform/mps_slow_ops.h"
#include "orteaf/internal/runtime/mps/platform/wrapper/mps_event.h"

namespace orteaf::internal::runtime::mps::manager {

// =============================================================================
// Pools (payload + control block)
// =============================================================================

struct EventPayloadPoolTraits {
  using Payload = ::orteaf::internal::runtime::mps::platform::wrapper::MpsEvent_t;
  using Handle = ::orteaf::internal::base::EventHandle;
  using DeviceType =
      ::orteaf::internal::runtime::mps::platform::wrapper::MpsDevice_t;
  using SlowOps = ::orteaf::internal::runtime::mps::platform::MpsSlowOps;

  struct Request {
    Handle handle{Handle::invalid()};
  };

  struct Context {
    DeviceType device{nullptr};
    SlowOps *ops{nullptr};
  };

  struct Config {
    std::size_t capacity{0};
  };

  static bool create(Payload &payload, const Request &, const Context &context) {
    if (context.ops == nullptr || context.device == nullptr) {
      return false;
    }
    auto event = context.ops->createEvent(context.device);
    if (event == nullptr) {
      return false;
    }
    payload = event;
    return true;
  }

  static void destroy(Payload &payload, const Request &,
                      const Context &context) {
    if (payload != nullptr && context.ops != nullptr) {
      context.ops->destroyEvent(payload);
      payload = nullptr;
    }
  }
};

using EventPayloadPool =
    ::orteaf::internal::runtime::base::pool::SlotPool<EventPayloadPoolTraits>;

struct EventControlBlockTag {};
using EventControlBlockHandle =
    ::orteaf::internal::base::ControlBlockHandle<EventControlBlockTag>;

using EventControlBlock =
    ::orteaf::internal::runtime::base::SharedControlBlock<
        ::orteaf::internal::base::EventHandle,
        ::orteaf::internal::runtime::mps::platform::wrapper::MpsEvent_t,
        EventPayloadPool>;

struct EventControlBlockPoolTraits {
  using Payload = EventControlBlock;
  using Handle = EventControlBlockHandle;
  struct Request {};
  struct Context {};
  struct Config {
    std::size_t capacity{0};
  };

  static bool create(Payload &, const Request &, const Context &) {
    return true;
  }

  static void destroy(Payload &, const Request &, const Context &) {}
};

using EventControlBlockPool =
    ::orteaf::internal::runtime::base::pool::SlotPool<
        EventControlBlockPoolTraits>;

class MpsEventManager {
public:
  using SlowOps = ::orteaf::internal::runtime::mps::platform::MpsSlowOps;
  using DeviceType =
      ::orteaf::internal::runtime::mps::platform::wrapper::MpsDevice_t;
  using EventHandle = ::orteaf::internal::base::EventHandle;
  using EventType =
      ::orteaf::internal::runtime::mps::platform::wrapper::MpsEvent_t;
  using ControlBlock = EventControlBlock;
  using ControlBlockHandle = EventControlBlockHandle;

  using EventLease = ::orteaf::internal::runtime::base::StrongLease<
      ControlBlockHandle, ControlBlock, EventControlBlockPool,
      MpsEventManager>;

private:
  friend EventLease;

public:
  MpsEventManager() = default;
  MpsEventManager(const MpsEventManager &) = delete;
  MpsEventManager &operator=(const MpsEventManager &) = delete;
  MpsEventManager(MpsEventManager &&) = default;
  MpsEventManager &operator=(MpsEventManager &&) = default;
  ~MpsEventManager() = default;

  void initialize(DeviceType device, SlowOps *ops, std::size_t capacity);
  void shutdown();

  EventLease acquire();
  void release(EventLease &lease) noexcept { lease.release(); }

  // Expose capacity
  std::size_t capacity() const noexcept { return payload_pool_.capacity(); }
  bool isInitialized() const noexcept { return initialized_; }
  bool isAlive(EventHandle handle) const noexcept;

#if ORTEAF_ENABLE_TEST
  std::size_t controlBlockPoolCapacityForTest() const noexcept {
    return control_block_pool_.capacity();
  }
#endif

private:
  EventPayloadPoolTraits::Context makePayloadContext() const noexcept;
  void ensureInitialized() const;
  void growPools(std::size_t desired_capacity);
  EventLease buildLease(ControlBlock &cb, EventHandle payload_handle,
                        ControlBlockHandle cb_handle);

  DeviceType device_{nullptr};
  SlowOps *ops_{nullptr};
  std::size_t growth_chunk_size_{1};
  bool initialized_{false};
  EventPayloadPool payload_pool_{};
  EventControlBlockPool control_block_pool_{};
};

} // namespace orteaf::internal::runtime::mps::manager

#endif // ORTEAF_ENABLE_MPS
