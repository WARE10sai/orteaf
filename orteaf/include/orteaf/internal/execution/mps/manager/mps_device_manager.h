#pragma once

#if ORTEAF_ENABLE_MPS

#include <cstddef>
#include <cstdint>
#include <utility>

#include "orteaf/internal/architecture/architecture.h"
#include "orteaf/internal/base/lease/control_block/strong.h"
#include "orteaf/internal/base/manager/lease_lifetime_registry.h"
#include "orteaf/internal/base/manager/pool_manager.h"
#include "orteaf/internal/base/pool/fixed_slot_store.h"
#include "orteaf/internal/diagnostics/error/error.h"
#include "orteaf/internal/execution/mps/mps_handles.h"
#include "orteaf/internal/execution/mps/manager/mps_command_queue_manager.h"
#include "orteaf/internal/execution/mps/manager/mps_event_manager.h"
#include "orteaf/internal/execution/mps/manager/mps_fence_manager.h"
#include "orteaf/internal/execution/mps/manager/mps_graph_manager.h"
#include "orteaf/internal/execution/mps/manager/mps_heap_manager.h"
#include "orteaf/internal/execution/mps/manager/mps_library_manager.h"
#include "orteaf/internal/execution/mps/platform/mps_slow_ops.h"
#include "orteaf/internal/execution/mps/platform/wrapper/mps_device.h"

namespace orteaf::internal::execution::mps::manager {

class MpsExecutionManager;

// =============================================================================
// Device Payload
// =============================================================================

struct MpsDevicePayload {
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;
  using DeviceHandle = ::orteaf::internal::execution::mps::MpsDeviceHandle;
  using DeviceType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsDevice_t;
  using Architecture = ::orteaf::internal::architecture::Architecture;

  struct InitConfig {
    SlowOps *ops{nullptr};
    DeviceHandle handle{DeviceHandle::invalid()};
    MpsCommandQueueManager::Config command_queue_config{};
    MpsEventManager::Config event_config{};
    MpsFenceManager::Config fence_config{};
    MpsHeapManager::Config heap_config{};
    MpsLibraryManager::Config library_config{};
    MpsGraphManager::Config graph_config{};
  };

  MpsDevicePayload() = default;
  MpsDevicePayload(const MpsDevicePayload &) = delete;
  MpsDevicePayload &operator=(const MpsDevicePayload &) = delete;

  MpsDevicePayload(MpsDevicePayload &&other) noexcept {
    moveFrom(std::move(other));
  }

  MpsDevicePayload &operator=(MpsDevicePayload &&other) noexcept {
    if (this != &other) {
      reset(nullptr);
      moveFrom(std::move(other));
    }
    return *this;
  }

  ~MpsDevicePayload() { reset(nullptr); }

  bool initialize(const InitConfig &config) {
    if (config.ops == nullptr || !config.handle.isValid()) {
      return false;
    }
    const auto device = config.ops->getDevice(
        static_cast<
            ::orteaf::internal::execution::mps::platform::wrapper::MPSInt_t>(
            config.handle.index));
    if (device == nullptr) {
      device_ = nullptr;
      arch_ = Architecture::MpsGeneric;
      return false;
    }
    device_ = device;
    arch_ = config.ops->detectArchitecture(config.handle);

    MpsFenceManager::InternalConfig fence_config{};
    fence_config.public_config = config.fence_config;
    fence_config.device = device;
    fence_config.ops = config.ops;
    fence_manager_.configure(fence_config);

    MpsCommandQueueManager::InternalConfig command_queue_config{};
    command_queue_config.public_config = config.command_queue_config;
    command_queue_config.device = device;
    command_queue_config.ops = config.ops;
    command_queue_config.fence_manager = &fence_manager_;
    command_queue_manager_.configure(command_queue_config);

    MpsLibraryManager::InternalConfig library_config{};
    library_config.public_config = config.library_config;
    library_config.device = device;
    library_config.ops = config.ops;
    library_manager_.configure(library_config);

    MpsHeapManager::InternalConfig heap_config{};
    heap_config.public_config = config.heap_config;
    heap_config.device = device;
    heap_config.device_handle = config.handle;
    heap_config.library_manager = &library_manager_;
    heap_config.ops = config.ops;
    heap_manager_.configure(heap_config);

    MpsGraphManager::InternalConfig graph_config{};
    graph_config.public_config = config.graph_config;
    graph_config.device = device;
    graph_config.ops = config.ops;
    graph_manager_.configure(graph_config);

    MpsEventManager::InternalConfig event_config{};
    event_config.public_config = config.event_config;
    event_config.device = device;
    event_config.ops = config.ops;
    event_manager_.configure(event_config);
    return true;
  }

  void reset(SlowOps *slow_ops) noexcept {
    command_queue_manager_.shutdown();
    heap_manager_.shutdown();
    library_manager_.shutdown();
    graph_manager_.shutdown();
    event_manager_.shutdown();
    fence_manager_.shutdown();
    if (device_ != nullptr && slow_ops != nullptr) {
      slow_ops->releaseDevice(device_);
    }
    device_ = nullptr;
    arch_ = Architecture::MpsGeneric;
  }

  DeviceType device() const noexcept { return device_; }
  Architecture architecture() const noexcept { return arch_; }

  // Accessor methods for encapsulated managers
  MpsLibraryManager &libraryManager() noexcept { return library_manager_; }
  const MpsLibraryManager &libraryManager() const noexcept {
    return library_manager_;
  }

  MpsCommandQueueManager &commandQueueManager() noexcept {
    return command_queue_manager_;
  }
  const MpsCommandQueueManager &commandQueueManager() const noexcept {
    return command_queue_manager_;
  }

  MpsHeapManager &heapManager() noexcept { return heap_manager_; }
  const MpsHeapManager &heapManager() const noexcept { return heap_manager_; }

  MpsGraphManager &graphManager() noexcept { return graph_manager_; }
  const MpsGraphManager &graphManager() const noexcept { return graph_manager_; }

  MpsEventManager &eventPool() noexcept { return event_manager_; }
  const MpsEventManager &eventPool() const noexcept { return event_manager_; }

  MpsFenceManager &fencePool() noexcept { return fence_manager_; }
  const MpsFenceManager &fencePool() const noexcept { return fence_manager_; }

private:
  void moveFrom(MpsDevicePayload &&other) noexcept {
    command_queue_manager_ = std::move(other.command_queue_manager_);
    heap_manager_ = std::move(other.heap_manager_);
    library_manager_ = std::move(other.library_manager_);
    graph_manager_ = std::move(other.graph_manager_);
    event_manager_ = std::move(other.event_manager_);
    fence_manager_ = std::move(other.fence_manager_);
    device_ = other.device_;
    arch_ = other.arch_;
    other.device_ = nullptr;
    other.arch_ = Architecture::MpsGeneric;
  }

  DeviceType device_{nullptr};
  Architecture arch_{Architecture::MpsGeneric};
  MpsCommandQueueManager command_queue_manager_{};
  MpsHeapManager heap_manager_{};
  MpsLibraryManager library_manager_{};
  MpsGraphManager graph_manager_{};
  MpsEventManager event_manager_{};
  MpsFenceManager fence_manager_{};
};

// =============================================================================
// Payload Pool
// =============================================================================

struct DevicePayloadPoolTraits {
  using Payload = MpsDevicePayload;
  using Handle = ::orteaf::internal::execution::mps::MpsDeviceHandle;
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;

  struct Request {
    Handle handle{Handle::invalid()};
  };

  struct Context {
    SlowOps *ops{nullptr};
    MpsCommandQueueManager::Config command_queue_config{};
    MpsEventManager::Config event_config{};
    MpsFenceManager::Config fence_config{};
    MpsHeapManager::Config heap_config{};
    MpsLibraryManager::Config library_config{};
    MpsGraphManager::Config graph_config{};
  };

  static bool create(Payload &payload, const Request &request,
                     const Context &context) {
    MpsDevicePayload::InitConfig init{};
    init.ops = context.ops;
    init.handle = request.handle;
    init.command_queue_config = context.command_queue_config;
    init.event_config = context.event_config;
    init.fence_config = context.fence_config;
    init.heap_config = context.heap_config;
    init.library_config = context.library_config;
    init.graph_config = context.graph_config;
    return payload.initialize(init);
  }

  static void destroy(Payload &payload, const Request &,
                      const Context &context) {
    payload.reset(context.ops);
  }
};

// Forward declare to get proper ordering
struct MpsDeviceManagerTraits;

// =============================================================================
// Payload Pool
// =============================================================================

using DevicePayloadPool =
    ::orteaf::internal::base::pool::FixedSlotStore<DevicePayloadPoolTraits>;

// Forward-declare CB tag to avoid circular dependency
struct DeviceManagerCBTag {};

// =============================================================================
// ControlBlock (using default pool traits via PoolManager)
// =============================================================================

using DeviceControlBlock = ::orteaf::internal::base::StrongControlBlock<
    ::orteaf::internal::execution::mps::MpsDeviceHandle, MpsDevicePayload,
    DevicePayloadPool>;

// =============================================================================
// Manager Traits for PoolManager
// =============================================================================

struct MpsDeviceManagerTraits {
  using PayloadPool = DevicePayloadPool;
  using ControlBlock = DeviceControlBlock;
  using ControlBlockTag = DeviceManagerCBTag; // Use the same tag
  using PayloadHandle = ::orteaf::internal::execution::mps::MpsDeviceHandle;
  static constexpr const char *Name = "MPS device manager";
};

// =============================================================================
// MpsDeviceManager
// =============================================================================

class MpsDeviceManager {
public:
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;
  using DeviceHandle = ::orteaf::internal::execution::mps::MpsDeviceHandle;
  using DeviceType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsDevice_t;

  using Core = ::orteaf::internal::base::PoolManager<MpsDeviceManagerTraits>;
  using ControlBlock = Core::ControlBlock;
  using ControlBlockHandle = Core::ControlBlockHandle;
  using ControlBlockPool = Core::ControlBlockPool;

  using DeviceLease = Core::StrongLeaseType;
  using LifetimeRegistry =
      ::orteaf::internal::base::manager::LeaseLifetimeRegistry<DeviceHandle,
                                                               DeviceLease>;

  struct Config {
    // PoolManager settings
    std::size_t control_block_capacity{0};
    std::size_t control_block_block_size{0};
    std::size_t control_block_growth_chunk_size{1};
    std::size_t payload_capacity{0};
    std::size_t payload_block_size{0};
    std::size_t payload_growth_chunk_size{1};
    MpsCommandQueueManager::Config command_queue_config{};
    MpsEventManager::Config event_config{};
    MpsFenceManager::Config fence_config{};
    MpsHeapManager::Config heap_config{};
    MpsLibraryManager::Config library_config{};
    MpsGraphManager::Config graph_config{};
  };

  MpsDeviceManager() = default;
  MpsDeviceManager(const MpsDeviceManager &) = delete;
  MpsDeviceManager &operator=(const MpsDeviceManager &) = delete;
  MpsDeviceManager(MpsDeviceManager &&) = default;
  MpsDeviceManager &operator=(MpsDeviceManager &&) = default;
  ~MpsDeviceManager() = default;

private:
  struct InternalConfig {
    Config public_config{};
    SlowOps *ops{nullptr};
  };

  void configure(const InternalConfig &config);

  friend class MpsExecutionManager;

public:
  // =========================================================================
  // Lifecycle
  // =========================================================================
  void shutdown();

  // =========================================================================
  // Device access
  // =========================================================================
  DeviceLease acquire(DeviceHandle handle);

#if ORTEAF_ENABLE_TEST
  void configureForTest(const Config &config, SlowOps *ops) {
    InternalConfig internal{};
    internal.public_config = config;
    internal.ops = ops;
    configure(internal);
  }

  std::size_t getDeviceCountForTest() const noexcept {
    return core_.payloadPoolSizeForTest();
  }
  bool isConfiguredForTest() const noexcept { return core_.isConfigured(); }
  std::size_t payloadPoolSizeForTest() const noexcept {
    return core_.payloadPoolSizeForTest();
  }
  std::size_t payloadPoolCapacityForTest() const noexcept {
    return core_.payloadPoolCapacityForTest();
  }
  bool isAliveForTest(DeviceHandle handle) const noexcept {
    return core_.isAlive(handle);
  }
  std::size_t controlBlockPoolAvailableForTest() const noexcept {
    return core_.controlBlockPoolAvailableForTest();
  }
#endif

private:
  SlowOps *ops_{nullptr};
  Core core_{};
  LifetimeRegistry lifetime_{};
};

} // namespace orteaf::internal::execution::mps::manager

#endif // ORTEAF_ENABLE_MPS
