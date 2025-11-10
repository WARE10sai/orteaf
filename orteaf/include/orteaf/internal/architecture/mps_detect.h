#pragma once

#include "orteaf/internal/architecture/architecture.h"

#include <string_view>

namespace orteaf::internal::architecture {

/// Detect the MPS (Metal) architecture using the reported Metal family (e.g., "m3") and optional vendor hint.
Architecture detect_mps_architecture(std::string_view metal_family, std::string_view vendor_hint = "apple");

} // namespace orteaf::internal::architecture
