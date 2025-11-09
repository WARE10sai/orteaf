#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include <orteaf/internal/architecture/architecture.h>
#include <orteaf/internal/backend/backend.h>
#include <orteaf/internal/dtype/dtype.h>
#include <orteaf/internal/ops/ops.h>

namespace orteaf::internal::device {

enum class Device : std::uint16_t {
#define DEVICE(ID, DISPLAY_NAME) ID,
#include <orteaf/device/device.def>
#undef DEVICE
    Count,
};

constexpr std::size_t ToIndex(Device device) {
    return static_cast<std::size_t>(device);
}

inline constexpr std::size_t kDeviceCount = static_cast<std::size_t>(Device::Count);

}  // namespace orteaf::internal::device

#include <orteaf/device/device_tables.h>

namespace orteaf::internal::device {

namespace tables = ::orteaf::generated::device_tables;

static_assert(kDeviceCount == tables::kDeviceCount,
              "Device enum size must match generated table size");
static_assert(tables::kDeviceDTypeOffsets.size() == kDeviceCount + 1,
              "Device dtype offset table must be device_count + 1");
static_assert(tables::kDeviceOpOffsets.size() == kDeviceCount + 1,
              "Device op offset table must be device_count + 1");
static_assert(tables::kDeviceCapabilityOffsets.size() == kDeviceCount + 1,
              "Device capability offset table must be device_count + 1");

inline constexpr std::array<std::string_view, kDeviceCount> kDeviceIds = {
#define DEVICE(ID, DISPLAY_NAME) std::string_view{#ID},
#include <orteaf/device/device.def>
#undef DEVICE
};

inline constexpr std::array<std::string_view, kDeviceCount> kDeviceDisplayNames = {
#define DEVICE(ID, DISPLAY_NAME) std::string_view{DISPLAY_NAME},
#include <orteaf/device/device.def>
#undef DEVICE
};

inline constexpr std::array<Device, kDeviceCount> kAllDevices = {
#define DEVICE(ID, DISPLAY_NAME) Device::ID,
#include <orteaf/device/device.def>
#undef DEVICE
};

struct MemoryInfo {
    std::uint64_t max_bytes;
    std::uint64_t shared_bytes;
};

struct Capability {
    std::string_view key;
    std::string_view value;
};

constexpr backend::Backend BackendOf(Device device) {
    return backend::FromIndex(tables::kDeviceBackendIndices[ToIndex(device)]);
}

constexpr architecture::Architecture ArchitectureOf(Device device) {
    const auto backend_id = BackendOf(device);
    const auto local_index = tables::kDeviceArchitectureLocalIndices[ToIndex(device)];
    return architecture::FromBackendAndLocalIndex(backend_id, local_index);
}

constexpr bool IsGeneric(Device device) {
    return tables::kDeviceArchitectureLocalIndices[ToIndex(device)] == 0;
}

constexpr MemoryInfo MemoryOf(Device device) {
    const auto index = ToIndex(device);
    return MemoryInfo{
        tables::kDeviceMemoryMaxBytes[index],
        tables::kDeviceMemorySharedBytes[index],
    };
}

constexpr std::string_view NotesOf(Device device) {
    return tables::kDeviceNotes[ToIndex(device)];
}

inline constexpr auto kDeviceDTypeEntries = [] {
    std::array<::orteaf::internal::DType, tables::kDeviceDTypeEntryCount> entries{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        entries[i] = ::orteaf::internal::FromIndex(tables::kDeviceDTypeIndices[i]);
    }
    return entries;
}();

inline constexpr auto kDeviceOpEntries = [] {
    std::array<::orteaf::internal::ops::Op, tables::kDeviceOpEntryCount> entries{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        entries[i] = ::orteaf::internal::ops::FromIndex(tables::kDeviceOpIndices[i]);
    }
    return entries;
}();

inline constexpr auto kDeviceCapabilityEntries = [] {
    std::array<Capability, tables::kDeviceCapabilityEntryCount> entries{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        entries[i] = Capability{
            tables::kDeviceCapabilityEntries[i].key,
            tables::kDeviceCapabilityEntries[i].value,
        };
    }
    return entries;
}();

inline constexpr std::span<const ::orteaf::internal::DType> SupportedDTypes(Device device) {
    const auto index = ToIndex(device);
    const auto begin = tables::kDeviceDTypeOffsets[index];
    const auto end = tables::kDeviceDTypeOffsets[index + 1];
    return std::span<const ::orteaf::internal::DType>(kDeviceDTypeEntries.data() + begin,
                                                      end - begin);
}

inline constexpr std::span<const ::orteaf::internal::ops::Op> SupportedOps(Device device) {
    const auto index = ToIndex(device);
    const auto begin = tables::kDeviceOpOffsets[index];
    const auto end = tables::kDeviceOpOffsets[index + 1];
    return std::span<const ::orteaf::internal::ops::Op>(kDeviceOpEntries.data() + begin,
                                                        end - begin);
}

inline constexpr std::span<const Capability> CapabilitiesOf(Device device) {
    const auto index = ToIndex(device);
    const auto begin = tables::kDeviceCapabilityOffsets[index];
    const auto end = tables::kDeviceCapabilityOffsets[index + 1];
    return std::span<const Capability>(kDeviceCapabilityEntries.data() + begin, end - begin);
}

inline constexpr std::span<const Device> AllDevices() {
    return std::span<const Device>(kAllDevices.data(), kAllDevices.size());
}

inline constexpr std::span<const std::string_view> AllDeviceIds() {
    return std::span<const std::string_view>(kDeviceIds.data(), kDeviceIds.size());
}

}  // namespace orteaf::internal::device
