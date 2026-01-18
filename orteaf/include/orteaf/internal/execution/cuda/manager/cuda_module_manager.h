#pragma once

#if ORTEAF_ENABLE_CUDA

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

#include "orteaf/internal/base/lease/control_block/strong.h"
#include "orteaf/internal/base/lease/strong_lease.h"
#include "orteaf/internal/base/manager/lease_lifetime_registry.h"
#include "orteaf/internal/base/manager/pool_manager.h"
#include "orteaf/internal/base/pool/slot_pool.h"
#include "orteaf/internal/execution/cuda/cuda_handles.h"
#include "orteaf/internal/execution/cuda/platform/cuda_slow_ops.h"
#include "orteaf/internal/execution/cuda/platform/wrapper/cuda_kernel_embed_api.h"
#include "orteaf/internal/execution/cuda/platform/wrapper/cuda_module.h"

namespace orteaf::internal::execution::cuda::manager {

struct ContextPayloadPoolTraits;

enum class ModuleKeyKind : std::uint8_t {
  kFile,
  kEmbedded,
};

struct ModuleKey {
  ModuleKeyKind kind{ModuleKeyKind::kFile};
  std::string identifier{};
  ::orteaf::internal::execution::cuda::platform::wrapper::kernel_embed::CudaKernelFmt
      preferred_format{::orteaf::internal::execution::cuda::platform::wrapper::kernel_embed::CudaKernelFmt::Fatbin};

  static ModuleKey File(std::string path) {
    ModuleKey key{};
    key.kind = ModuleKeyKind::kFile;
    key.identifier = std::move(path);
    return key;
  }

  static ModuleKey Embedded(std::string name,
                            ::orteaf::internal::execution::cuda::platform::wrapper::kernel_embed::CudaKernelFmt
                                preferred = ::orteaf::internal::execution::cuda::platform::wrapper::kernel_embed::CudaKernelFmt::Fatbin) {
    ModuleKey key{};
    key.kind = ModuleKeyKind::kEmbedded;
    key.identifier = std::move(name);
    key.preferred_format = preferred;
    return key;
  }

  friend bool operator==(const ModuleKey &lhs,
                         const ModuleKey &rhs) noexcept = default;
};

struct ModuleKeyHasher {
  std::size_t operator()(const ModuleKey &key) const noexcept {
    std::size_t seed = static_cast<std::size_t>(key.kind);
    constexpr std::size_t kMagic = 0x9e3779b97f4a7c15ull;
    seed ^= std::hash<std::string>{}(key.identifier) + kMagic + (seed << 6) +
            (seed >> 2);
    seed ^= static_cast<std::size_t>(key.preferred_format) + kMagic +
            (seed << 6) + (seed >> 2);
    return seed;
  }
};

struct CudaModuleResource {
  ::orteaf::internal::execution::cuda::platform::wrapper::CudaModule_t module{
      nullptr};
  std::unordered_map<std::string,
                     ::orteaf::internal::execution::cuda::platform::wrapper::CudaFunction_t>
      function_cache{};
};

struct ModulePayloadPoolTraits {
  using Payload = CudaModuleResource;
  using Handle = ::orteaf::internal::execution::cuda::CudaModuleHandle;
  using ContextType =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaContext_t;
  using SlowOps = ::orteaf::internal::execution::cuda::platform::CudaSlowOps;

  using LoadFromFileFn =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaModule_t (*)(
          const char *);
  using LoadFromImageFn =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaModule_t (*)(
          const void *);
  using GetFunctionFn =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaFunction_t (*)(
          ::orteaf::internal::execution::cuda::platform::wrapper::CudaModule_t,
          const char *);
  using UnloadFn = void (*)(
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaModule_t);

  struct Request {
    ModuleKey key{};
  };

  struct Context {
    ContextType context{nullptr};
    SlowOps *ops{nullptr};
    LoadFromFileFn load_from_file{nullptr};
    LoadFromImageFn load_from_image{nullptr};
    GetFunctionFn get_function{nullptr};
    UnloadFn unload{nullptr};
  };

  static bool create(Payload &payload, const Request &request,
                     const Context &context);
  static void destroy(Payload &payload, const Request &,
                      const Context &context);
};

using ModulePayloadPool =
    ::orteaf::internal::base::pool::SlotPool<ModulePayloadPoolTraits>;

struct ModuleControlBlockTag {};

using ModuleControlBlock = ::orteaf::internal::base::StrongControlBlock<
    ::orteaf::internal::execution::cuda::CudaModuleHandle, CudaModuleResource,
    ModulePayloadPool>;

struct CudaModuleManagerTraits {
  using PayloadPool = ModulePayloadPool;
  using ControlBlock = ModuleControlBlock;
  struct ControlBlockTag {};
  using PayloadHandle = ::orteaf::internal::execution::cuda::CudaModuleHandle;
  static constexpr const char *Name = "CUDA module manager";
};

class CudaModuleManager {
public:
  using Core = ::orteaf::internal::base::PoolManager<CudaModuleManagerTraits>;
  using SlowOps = ::orteaf::internal::execution::cuda::platform::CudaSlowOps;
  using ContextType =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaContext_t;
  using ModuleHandle = ::orteaf::internal::execution::cuda::CudaModuleHandle;
  using ModuleType =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaModule_t;
  using FunctionType =
      ::orteaf::internal::execution::cuda::platform::wrapper::CudaFunction_t;
  using ModuleLease = Core::StrongLeaseType;
  using LifetimeRegistry =
      ::orteaf::internal::base::manager::LeaseLifetimeRegistry<ModuleHandle,
                                                               ModuleLease>;

  struct Config {
    ModulePayloadPoolTraits::LoadFromFileFn load_from_file{nullptr};
    ModulePayloadPoolTraits::LoadFromImageFn load_from_image{nullptr};
    ModulePayloadPoolTraits::GetFunctionFn get_function{nullptr};
    ModulePayloadPoolTraits::UnloadFn unload{nullptr};
    std::size_t control_block_capacity{0};
    std::size_t control_block_block_size{0};
    std::size_t control_block_growth_chunk_size{1};
    std::size_t payload_capacity{0};
    std::size_t payload_block_size{0};
    std::size_t payload_growth_chunk_size{1};
  };

  CudaModuleManager() = default;
  CudaModuleManager(const CudaModuleManager &) = delete;
  CudaModuleManager &operator=(const CudaModuleManager &) = delete;
  CudaModuleManager(CudaModuleManager &&) = default;
  CudaModuleManager &operator=(CudaModuleManager &&) = default;
  ~CudaModuleManager() = default;

private:
  struct InternalConfig {
    Config public_config{};
    ContextType context{nullptr};
    SlowOps *ops{nullptr};
  };

  void configure(const InternalConfig &config);

  friend struct ContextPayloadPoolTraits;

public:
  void shutdown();

  ModuleLease acquire(const ModuleKey &key);
  ModuleLease acquire(ModuleHandle handle);

  FunctionType getFunction(ModuleLease &lease, std::string_view name);

#if ORTEAF_ENABLE_TEST
  void configureForTest(const Config &config, ContextType context,
                        SlowOps *ops) {
    InternalConfig internal{};
    internal.public_config = config;
    internal.context = context;
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
  bool isAliveForTest(ModuleHandle handle) const noexcept {
    return core_.isAlive(handle);
  }
#endif

private:
  void validateKey(const ModuleKey &key) const;
  ModulePayloadPoolTraits::Context makePayloadContext() const noexcept;

  ContextType context_{nullptr};
  SlowOps *ops_{nullptr};
  ModulePayloadPoolTraits::LoadFromFileFn load_from_file_{nullptr};
  ModulePayloadPoolTraits::LoadFromImageFn load_from_image_{nullptr};
  ModulePayloadPoolTraits::GetFunctionFn get_function_{nullptr};
  ModulePayloadPoolTraits::UnloadFn unload_{nullptr};
  Core core_{};
  LifetimeRegistry lifetime_{};
  std::unordered_map<ModuleKey, std::size_t, ModuleKeyHasher> key_to_index_{};
};

} // namespace orteaf::internal::execution::cuda::manager

#endif // ORTEAF_ENABLE_CUDA
