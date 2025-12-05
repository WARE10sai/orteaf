#pragma once

#if ORTEAF_ENABLE_MPS

namespace orteaf::internal::runtime::ops::mps {

class MpsCommonOps {
public:
    MpsCommonOps() = default;
    MpsCommonOps(const MpsCommonOps&) = default;
    MpsCommonOps& operator=(const MpsCommonOps&) = default;
    MpsCommonOps(MpsCommonOps&&) = default;
    MpsCommonOps& operator=(MpsCommonOps&&) = default;
    ~MpsCommonOps() = default;
};

}  // namespace orteaf::internal::runtime::ops::mps

#endif  // ORTEAF_ENABLE_MPS

