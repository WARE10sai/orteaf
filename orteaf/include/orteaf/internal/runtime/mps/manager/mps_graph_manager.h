#pragma once

#if ORTEAF_ENABLE_MPS

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "orteaf/internal/base/handle.h"
#include "orteaf/internal/base/lease.h"
#include "orteaf/internal/runtime/base/shared_cache_manager.h"
#include "orteaf/internal/runtime/mps/platform/mps_slow_ops.h"
#include "orteaf/internal/runtime/mps/platform/wrapper/mps_graph.h"

namespace orteaf::internal::runtime::mps::manager {

enum class GraphKeyKind : std::uint8_t { kNamed };

struct GraphKey {
  GraphKeyKind kind{GraphKeyKind::kNamed};
  std::string identifier{};
  std::vector<std::int64_t> shape{};
  ::orteaf::internal::runtime::mps::platform::wrapper::MpsGraphDataType
      data_type{::orteaf::internal::runtime::mps::platform::wrapper::
                    MpsGraphDataType::kInvalid};
  std::size_t target_tensor_count{0};
  bool has_gradients{false};

  static GraphKey Named(std::string identifier) {
    GraphKey key{};
    key.kind = GraphKeyKind::kNamed;
    key.identifier = std::move(identifier);
    return key;
  }

  friend bool operator==(const GraphKey &lhs,
                         const GraphKey &rhs) noexcept = default;
};

struct GraphKeyHasher {
  std::size_t operator()(const GraphKey &key) const noexcept {
    std::size_t seed = static_cast<std::size_t>(key.kind);
    constexpr std::size_t kMagic = 0x9e3779b97f4a7c15ull;
    seed ^= std::hash<std::string>{}(key.identifier) + kMagic + (seed << 6) +
            (seed >> 2);
    seed ^= std::hash<std::size_t>{}(key.shape.size()) + kMagic + (seed << 6) +
            (seed >> 2);
    for (auto dim : key.shape) {
      seed ^=
          std::hash<std::int64_t>{}(dim) + kMagic + (seed << 6) + (seed >> 2);
    }
    seed ^=
        std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(key.data_type)) +
        kMagic + (seed << 6) + (seed >> 2);
    seed ^= std::hash<std::size_t>{}(key.target_tensor_count) + kMagic +
            (seed << 6) + (seed >> 2);
    seed ^= std::hash<bool>{}(key.has_gradients) + kMagic + (seed << 6) +
            (seed >> 2);
    return seed;
  }
};

// Resource struct: holds graph + executable
struct MpsGraphResource {
  ::orteaf::internal::runtime::mps::platform::wrapper::MPSGraph_t graph{
      nullptr};
  ::orteaf::internal::runtime::mps::platform::wrapper::MPSGraphExecutable_t
      executable{nullptr};
};

// Use SharedCacheState template
using MpsGraphManagerState =
    ::orteaf::internal::runtime::base::SharedCacheState<MpsGraphResource>;

struct MpsGraphManagerTraits {
  using OpsType = ::orteaf::internal::runtime::mps::platform::MpsSlowOps;
  using StateType = MpsGraphManagerState;
  static constexpr const char *Name = "MPS graph manager";
};

class MpsGraphManager
    : public ::orteaf::internal::runtime::base::SharedCacheManager<
          MpsGraphManager, MpsGraphManagerTraits> {
public:
  using Base = ::orteaf::internal::runtime::base::SharedCacheManager<
      MpsGraphManager, MpsGraphManagerTraits>;
  using SlowOps = ::orteaf::internal::runtime::mps::platform::MpsSlowOps;
  using DeviceType =
      ::orteaf::internal::runtime::mps::platform::wrapper::MPSDevice_t;
  using GraphHandle = ::orteaf::internal::base::GraphHandle;
  using GraphLease = ::orteaf::internal::base::Lease<
      GraphHandle,
      ::orteaf::internal::runtime::mps::platform::wrapper::MPSGraphExecutable_t,
      MpsGraphManager>;
  using CompileFn = std::function<
      ::orteaf::internal::runtime::mps::platform::wrapper::MPSGraphExecutable_t(
          ::orteaf::internal::runtime::mps::platform::wrapper::MPSGraph_t graph,
          DeviceType device, SlowOps *slow_ops)>;

  MpsGraphManager() = default;
  MpsGraphManager(const MpsGraphManager &) = delete;
  MpsGraphManager &operator=(const MpsGraphManager &) = delete;
  MpsGraphManager(MpsGraphManager &&) = default;
  MpsGraphManager &operator=(MpsGraphManager &&) = default;
  ~MpsGraphManager() = default;

  void initialize(DeviceType device, SlowOps *ops, std::size_t capacity);
  void shutdown();

  GraphLease acquire(const GraphKey &key, const CompileFn &compile_fn);
  void release(GraphLease &lease) noexcept;

private:
  void validateKey(const GraphKey &key) const;
  void destroyResource(MpsGraphResource &resource);

  std::unordered_map<GraphKey, std::size_t, GraphKeyHasher> key_to_index_{};
  DeviceType device_{nullptr};
};

} // namespace orteaf::internal::runtime::mps::manager

#endif // ORTEAF_ENABLE_MPS
