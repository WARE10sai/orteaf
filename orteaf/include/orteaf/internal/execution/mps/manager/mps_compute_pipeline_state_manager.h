#pragma once

#if ORTEAF_ENABLE_MPS

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "orteaf/internal/base/lease/control_block/strong.h"
#include "orteaf/internal/base/manager/lease_lifetime_registry.h"
#include "orteaf/internal/base/manager/pool_manager.h"
#include "orteaf/internal/base/pool/fixed_slot_store.h"
#include "orteaf/internal/execution/mps/mps_handles.h"
#include "orteaf/internal/execution/mps/platform/mps_slow_ops.h"
#include "orteaf/internal/execution/mps/platform/wrapper/mps_compute_pipeline_state.h"
#include "orteaf/internal/execution/mps/platform/wrapper/mps_function.h"
#include "orteaf/internal/execution/mps/platform/wrapper/mps_library.h"

namespace orteaf::internal::execution::mps::manager {

struct LibraryPayloadPoolTraits;

enum class FunctionKeyKind : std::uint8_t {
  kNamed,
};

struct FunctionKey {
  FunctionKeyKind kind{FunctionKeyKind::kNamed};
  std::string identifier{};

  static FunctionKey Named(std::string identifier) {
    FunctionKey key{};
    key.kind = FunctionKeyKind::kNamed;
    key.identifier = std::move(identifier);
    return key;
  }

  friend bool operator==(const FunctionKey &lhs,
                         const FunctionKey &rhs) noexcept = default;
};

struct FunctionKeyHasher {
  std::size_t operator()(const FunctionKey &key) const noexcept {
    std::size_t seed = static_cast<std::size_t>(key.kind);
    constexpr std::size_t kMagic = 0x9e3779b97f4a7c15ull;
    seed ^= std::hash<std::string>{}(key.identifier) + kMagic + (seed << 6) +
            (seed >> 2);
    return seed;
  }
};

// Payload struct: holds function + pipeline_state
struct MpsPipelinePayload {
  using FunctionType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsFunction_t;
  using PipelineStateType = ::orteaf::internal::execution::mps::platform::
      wrapper::MpsComputePipelineState_t;
  using DeviceType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsDevice_t;
  using LibraryType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsLibrary_t;
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;

  struct InitConfig {
    DeviceType device{nullptr};
    LibraryType library{nullptr};
    SlowOps *ops{nullptr};
    FunctionKey key{};
  };

  MpsPipelinePayload() = default;
  MpsPipelinePayload(const MpsPipelinePayload &) = delete;
  MpsPipelinePayload &operator=(const MpsPipelinePayload &) = delete;
  MpsPipelinePayload(MpsPipelinePayload &&) = default;
  MpsPipelinePayload &operator=(MpsPipelinePayload &&) = default;
  ~MpsPipelinePayload() = default;

  bool initialize(const InitConfig &config) {
    if (config.ops == nullptr || config.device == nullptr ||
        config.library == nullptr) {
      return false;
    }
    function_ =
        config.ops->createFunction(config.library, config.key.identifier);
    if (function_ == nullptr) {
      return false;
    }
    pipeline_state_ =
        config.ops->createComputePipelineState(config.device, function_);
    if (pipeline_state_ == nullptr) {
      config.ops->destroyFunction(function_);
      function_ = nullptr;
      return false;
    }
    return true;
  }

  void reset(SlowOps *ops) noexcept {
    if (pipeline_state_ != nullptr && ops != nullptr) {
      ops->destroyComputePipelineState(pipeline_state_);
      pipeline_state_ = nullptr;
    }
    if (function_ != nullptr && ops != nullptr) {
      ops->destroyFunction(function_);
      function_ = nullptr;
    }
  }

  FunctionType function() const noexcept { return function_; }
  PipelineStateType pipelineState() const noexcept { return pipeline_state_; }

private:
  FunctionType function_{nullptr};
  PipelineStateType pipeline_state_{nullptr};
};

struct PipelinePayloadPoolTraits {
  using Payload = MpsPipelinePayload;
  using Handle = ::orteaf::internal::execution::mps::MpsFunctionHandle;
  using DeviceType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsDevice_t;
  using LibraryType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsLibrary_t;
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;

  struct Request {
    FunctionKey key{};
  };

  struct Context {
    DeviceType device{nullptr};
    LibraryType library{nullptr};
    SlowOps *ops{nullptr};
  };

  static bool create(Payload &payload, const Request &request,
                     const Context &context) {
    MpsPipelinePayload::InitConfig init{};
    init.device = context.device;
    init.library = context.library;
    init.ops = context.ops;
    init.key = request.key;
    return payload.initialize(init);
  }

  static void destroy(Payload &payload, const Request &,
                      const Context &context) {
    payload.reset(context.ops);
  }
};

using PipelinePayloadPool =
    ::orteaf::internal::base::pool::FixedSlotStore<PipelinePayloadPoolTraits>;

struct PipelineControlBlockTag {};

using PipelineControlBlock = ::orteaf::internal::base::StrongControlBlock<
    ::orteaf::internal::execution::mps::MpsFunctionHandle, MpsPipelinePayload,
    PipelinePayloadPool>;

struct MpsComputePipelineStateManagerTraits {
  using PayloadPool = PipelinePayloadPool;
  using ControlBlock = PipelineControlBlock;
  struct ControlBlockTag {};
  using PayloadHandle = ::orteaf::internal::execution::mps::MpsFunctionHandle;
  static constexpr const char *Name = "MpsComputePipelineStateManager";
};

class MpsComputePipelineStateManager {
  using Core = ::orteaf::internal::base::PoolManager<
      MpsComputePipelineStateManagerTraits>;

public:
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;
  using DeviceType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsDevice_t;
  using LibraryType =
      ::orteaf::internal::execution::mps::platform::wrapper::MpsLibrary_t;
  using FunctionHandle = ::orteaf::internal::execution::mps::MpsFunctionHandle;
  using PipelineState = ::orteaf::internal::execution::mps::platform::wrapper::
      MpsComputePipelineState_t;
  using ControlBlockHandle = Core::ControlBlockHandle;
  using ControlBlockPool = Core::ControlBlockPool;
  using PipelineLease = Core::StrongLeaseType;
  using LifetimeRegistry =
      ::orteaf::internal::base::manager::LeaseLifetimeRegistry<FunctionHandle,
                                                               PipelineLease>;

  MpsComputePipelineStateManager() = default;
  MpsComputePipelineStateManager(const MpsComputePipelineStateManager &) =
      delete;
  MpsComputePipelineStateManager &
  operator=(const MpsComputePipelineStateManager &) = delete;
  MpsComputePipelineStateManager(MpsComputePipelineStateManager &&) = default;
  MpsComputePipelineStateManager &
  operator=(MpsComputePipelineStateManager &&) = default;
  ~MpsComputePipelineStateManager() = default;

  struct Config {
    // PoolManager settings
    std::size_t control_block_capacity{0};
    std::size_t control_block_block_size{0};
    std::size_t control_block_growth_chunk_size{1};
    std::size_t payload_capacity{0};
    std::size_t payload_block_size{0};
    std::size_t payload_growth_chunk_size{1};
  };

private:
  struct InternalConfig {
    Config public_config{};
    DeviceType device{nullptr};
    LibraryType library{nullptr};
    SlowOps *ops{nullptr};
  };

  void configure(const InternalConfig &config);

  friend struct LibraryPayloadPoolTraits;
  friend struct MpsLibraryPayload;

public:
  void shutdown();

  PipelineLease acquire(const FunctionKey &key);

#if ORTEAF_ENABLE_TEST
  void configureForTest(const Config &config, DeviceType device,
                        LibraryType library, SlowOps *ops) {
    InternalConfig internal{};
    internal.public_config = config;
    internal.device = device;
    internal.library = library;
    internal.ops = ops;
    configure(internal);
  }

  bool isConfiguredForTest() const noexcept { return core_.isConfigured(); }

  std::size_t payloadPoolSizeForTest() const noexcept {
    return core_.payloadPoolSizeForTest();
  }
  std::size_t payloadPoolCapacityForTest() const noexcept {
    return core_.payloadPoolCapacityForTest();
  }
  std::size_t controlBlockPoolSizeForTest() const noexcept {
    return core_.controlBlockPoolSizeForTest();
  }
  std::size_t controlBlockPoolCapacityForTest() const noexcept {
    return core_.controlBlockPoolCapacityForTest();
  }
  bool isAliveForTest(FunctionHandle handle) const noexcept {
    return core_.isAlive(handle);
  }
  std::size_t payloadGrowthChunkSizeForTest() const noexcept {
    return core_.payloadGrowthChunkSize();
  }
  std::size_t controlBlockGrowthChunkSizeForTest() const noexcept {
    return core_.controlBlockGrowthChunkSize();
  }

  bool payloadCreatedForTest(FunctionHandle handle) const noexcept {
    return core_.payloadCreatedForTest(handle);
  }

  const MpsPipelinePayload *
  payloadForTest(FunctionHandle handle) const noexcept {
    return core_.payloadForTest(handle);
  }
#endif

private:
  void validateKey(const FunctionKey &key) const;
  PipelinePayloadPoolTraits::Context makePayloadContext() const noexcept;

  std::unordered_map<FunctionKey, std::size_t, FunctionKeyHasher> key_to_index_{};
  LibraryType library_{nullptr};
  DeviceType device_{nullptr};
  SlowOps *ops_{nullptr};
  Core core_{};
  LifetimeRegistry lifetime_{};
};

} // namespace orteaf::internal::execution::mps::manager

#endif // ORTEAF_ENABLE_MPS
