#include <orteaf/internal/kernel/kernel_storage_schema.h>

#include <gtest/gtest.h>

#include <orteaf/internal/kernel/cpu/cpu_kernel_args.h>
#include <orteaf/internal/kernel/cpu/cpu_storage_binding.h>
#include <orteaf/internal/kernel/storage_id.h>

namespace orteaf::internal::kernel {
namespace {

using ::orteaf::internal::kernel::cpu::CpuKernelArgs;
using ::orteaf::internal::kernel::cpu::CpuStorageBinding;

// Define storage schema outside test function
struct SimpleStorageSchema : StorageSchema<SimpleStorageSchema> {
  OptionalStorageField<StorageId::Input0> input;
  OptionalStorageField<StorageId::Output> output;

  ORTEAF_EXTRACT_STORAGES(input, output)
};

TEST(KernelStorageSchemaTest, BasicExtraction) {
  CpuKernelArgs args;

  // Extract from empty args
  auto schema = SimpleStorageSchema::extract(args);

  EXPECT_FALSE(schema.input);
  EXPECT_FALSE(schema.output);
  EXPECT_FALSE(schema.input.present());
  EXPECT_FALSE(schema.output.present());
}

struct OptionalStorageSchema : StorageSchema<OptionalStorageSchema> {
  OptionalStorageField<StorageId::Input0> input;
  OptionalStorageField<StorageId::Output> output;
  OptionalStorageField<StorageId::Workspace> workspace;

  ORTEAF_EXTRACT_STORAGES(input, output, workspace)
};

TEST(KernelStorageSchemaTest, OptionalStorageField) {
  CpuKernelArgs args;

  auto schema = OptionalStorageSchema::extract(args);

  EXPECT_FALSE(schema.input);
  EXPECT_FALSE(schema.output);
  EXPECT_FALSE(schema.workspace);

  EXPECT_FALSE(schema.output.present());
  EXPECT_FALSE(schema.workspace.present());

  // Optional field returns nullptr when not present
  auto *workspace_binding = schema.workspace.bindingOr<CpuStorageBinding>();
  EXPECT_EQ(workspace_binding, nullptr);
}

struct RequiredStorageSchema : StorageSchema<RequiredStorageSchema> {
  StorageField<StorageId::Input0> input;
  StorageField<StorageId::Output> output;

  ORTEAF_EXTRACT_STORAGES(input, output)
};

TEST(KernelStorageSchemaTest, MissingRequiredStorage) {
  CpuKernelArgs args;

  EXPECT_THROW(RequiredStorageSchema::extract(args), std::runtime_error);
}

TEST(KernelStorageSchemaTest, OptionalFieldNotPresent) {
  CpuKernelArgs args;

  // Single field extraction
  OptionalStorageField<StorageId::Workspace> workspace_field;
  workspace_field.extract(args);

  EXPECT_FALSE(workspace_field);
  EXPECT_FALSE(workspace_field.present());
}

} // namespace
} // namespace orteaf::internal::kernel

