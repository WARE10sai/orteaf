#pragma once

#if ORTEAF_ENABLE_MPS

namespace orteaf::internal::runtime::ops::mps {

class MpsPrivateOps {
public:
    MpsPrivateOps() = default;
    MpsPrivateOps(const MpsPrivateOps&) = default;
    MpsPrivateOps& operator=(const MpsPrivateOps&) = default;
    MpsPrivateOps(MpsPrivateOps&&) = default;
    MpsPrivateOps& operator=(MpsPrivateOps&&) = default;
    ~MpsPrivateOps() = default;
};

}  // namespace orteaf::internal::runtime::ops::mps

#endif  // ORTEAF_ENABLE_MPS

