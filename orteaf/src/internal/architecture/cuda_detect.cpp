#include "orteaf/internal/architecture/cuda_detect.h"

#include "orteaf/internal/backend/backend.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace orteaf::internal::architecture {

namespace {

namespace tables = ::orteaf::generated::architecture_tables;

std::string ToLowerCopy(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

bool MatchesVendor(std::string_view required, std::string_view actual_lower) {
    if (required.empty()) {
        return true;
    }
    return ToLowerCopy(required) == actual_lower;
}

} // namespace

Architecture detect_cuda_architecture(int compute_capability, std::string_view vendor_hint) {
    const auto vendor_lower = ToLowerCopy(vendor_hint);
    const auto count = tables::kArchitectureCount;
    Architecture fallback = Architecture::cuda_generic;

    for (std::size_t index = 0; index < count; ++index) {
        const Architecture arch = kAllArchitectures[index];
        if (LocalIndexOf(arch) == 0) {
            continue;
        }
        if (BackendOf(arch) != backend::Backend::cuda) {
            continue;
        }

        const auto required_vendor = tables::kArchitectureDetectVendors[index];
        if (!MatchesVendor(required_vendor, vendor_lower)) {
            continue;
        }

        const int required_cc = tables::kArchitectureDetectComputeCapability[index];
        if (required_cc >= 0 && required_cc != compute_capability) {
            continue;
        }

        return arch;
    }

    return fallback;
}

} // namespace orteaf::internal::architecture
