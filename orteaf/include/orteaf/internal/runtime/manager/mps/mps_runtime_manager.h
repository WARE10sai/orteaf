#pragma once

#if ORTEAF_ENABLE_MPS

#include "orteaf/internal/runtime/manager/mps/mps_device_manager.h"

namespace orteaf::internal::runtime::mps {

class MpsRuntimeManager {
public:
    MpsRuntimeManager() = default;
    MpsRuntimeManager(const MpsRuntimeManager&) = delete;
    MpsRuntimeManager& operator=(const MpsRuntimeManager&) = delete;
    MpsRuntimeManager(MpsRuntimeManager&&) = default;
    MpsRuntimeManager& operator=(MpsRuntimeManager&&) = default;
    ~MpsRuntimeManager() = default;

    MpsDeviceManager& deviceManager() noexcept { return device_manager_; }
    const MpsDeviceManager& deviceManager() const noexcept { return device_manager_; }

private:
    MpsDeviceManager device_manager_{};
};

inline MpsRuntimeManager& GetMpsRuntimeManager() {
    static MpsRuntimeManager instance{};
    return instance;
}

}  // namespace orteaf::internal::runtime::mps

#endif  // ORTEAF_ENABLE_MPS

