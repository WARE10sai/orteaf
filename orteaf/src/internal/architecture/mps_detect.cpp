#include "orteaf/internal/architecture/mps_detect.h"

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

bool MatchesVendor(std::string_view required, std::string_view hint_lower) {
    if (required.empty()) {
        return true;
    }
    return ToLowerCopy(required) == hint_lower;
}

} // namespace

Architecture detect_mps_architecture(std::string_view metal_family, std::string_view vendor_hint) {
    const auto metal_lower = ToLowerCopy(metal_family);
    const auto vendor_lower = ToLowerCopy(vendor_hint);
    const auto count = tables::kArchitectureCount;
    Architecture fallback = Architecture::mps_generic;

    for (std::size_t index = 0; index < count; ++index) {
        const Architecture arch = kAllArchitectures[index];
        if (LocalIndexOf(arch) == 0) {
            continue;
        }
        if (BackendOf(arch) != backend::Backend::mps) {
            continue;
        }

        const auto required_vendor = tables::kArchitectureDetectVendors[index];
        if (!MatchesVendor(required_vendor, vendor_lower)) {
            continue;
        }

        const auto required_family = tables::kArchitectureDetectMetalFamilies[index];
        if (!required_family.empty() && ToLowerCopy(required_family) != metal_lower) {
            continue;
        }

        return arch;
    }

    return fallback;
}

} // namespace orteaf::internal::architecture
