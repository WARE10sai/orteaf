#pragma once

#if ORTEAF_ENABLE_MPS

namespace orteaf::internal::runtime::ops::mps {

class MpsPublicOps {
public:
    MpsPublicOps() = default;
    MpsPublicOps(const MpsPublicOps&) = default;
    MpsPublicOps& operator=(const MpsPublicOps&) = default;
    MpsPublicOps(MpsPublicOps&&) = default;
    MpsPublicOps& operator=(MpsPublicOps&&) = default;
    ~MpsPublicOps() = default;
};

}  // namespace orteaf::internal::runtime::ops::mps

#endif  // ORTEAF_ENABLE_MPS

