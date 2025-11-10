#pragma once

#include "orteaf/internal/architecture/architecture.h"

#include <string_view>

namespace orteaf::internal::architecture {

/// Detect the CUDA architecture given a compute capability (e.g., 80 for SM80) and optional vendor hint.
Architecture detect_cuda_architecture(int compute_capability, std::string_view vendor_hint = "nvidia");

} // namespace orteaf::internal::architecture
