#pragma once

#if ORTEAF_ENABLE_MPS

#include <memory>

#include "orteaf/internal/execution/mps/manager/mps_device_manager.h"
#include "orteaf/internal/execution/mps/platform/mps_slow_ops.h"

namespace orteaf::internal::execution::mps::manager {

class MpsExecutionManager {
  using SlowOps = ::orteaf::internal::execution::mps::platform::MpsSlowOps;
  using SlowOpsImpl =
      ::orteaf::internal::execution::mps::platform::MpsSlowOpsImpl;

public:
  // =========================================================================
  // Config
  // =========================================================================

  struct Config {
    /// Custom SlowOps instance (nullptr for default implementation).
    /// If provided, the ExecutionManager takes ownership.
    SlowOps *slow_ops = nullptr;
    /// Device manager configuration
    MpsDeviceManager::Config device_config = {};
  };

  MpsExecutionManager() = default;
  MpsExecutionManager(const MpsExecutionManager &) = delete;
  MpsExecutionManager &operator=(const MpsExecutionManager &) = delete;
  MpsExecutionManager(MpsExecutionManager &&) = default;
  MpsExecutionManager &operator=(MpsExecutionManager &&) = default;
  ~MpsExecutionManager() = default;

  // =========================================================================
  // Manager accessors
  // =========================================================================

  MpsDeviceManager &deviceManager() noexcept { return device_manager_; }
  const MpsDeviceManager &deviceManager() const noexcept {
    return device_manager_;
  }

  SlowOps *slowOps() noexcept { return slow_ops_.get(); }
  const SlowOps *slowOps() const noexcept { return slow_ops_.get(); }

  // =========================================================================
  // Lifecycle
  // =========================================================================

  /**
   * @brief Configure the MPS execution manager with default settings.
   */
  void configure() { configure(Config{}); }

  /**
   * @brief Configure the MPS execution manager.
   *
   * @param config Configuration including SlowOps and sub-manager settings
   */
  void configure(const Config &config) {
    if (config.slow_ops) {
      slow_ops_.reset(config.slow_ops);
    } else if (!slow_ops_) {
      slow_ops_ = std::make_unique<SlowOpsImpl>();
    }

    // Configure device manager
    MpsDeviceManager::InternalConfig device_config{};
    device_config.public_config = config.device_config;
    device_config.ops = slow_ops_.get();
    device_manager_.configure(device_config);
  }

  void shutdown() {
    device_manager_.shutdown();
    slow_ops_.reset();
  }

  /**
   * @brief Check if the execution manager is configured.
   */
  bool isConfigured() const noexcept {
#if ORTEAF_ENABLE_TEST
    return slow_ops_ != nullptr && device_manager_.isConfiguredForTest();
#else
    return slow_ops_ != nullptr;
#endif
  }

private:
  MpsDeviceManager device_manager_{};
  std::unique_ptr<SlowOps> slow_ops_{};
};

} // namespace orteaf::internal::execution::mps::manager

#endif // ORTEAF_ENABLE_MPS
