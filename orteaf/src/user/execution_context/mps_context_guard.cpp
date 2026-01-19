#include "orteaf/user/execution_context/mps_context_guard.h"

#if ORTEAF_ENABLE_MPS

#include <utility>

namespace orteaf::user::execution_context {
namespace {

namespace mps_context = ::orteaf::internal::execution_context::mps;

} // namespace

MpsExecutionContextGuard::MpsExecutionContextGuard() {
  activate(mps_context::Context{
      ::orteaf::internal::execution::mps::MpsDeviceHandle{0}});
}

MpsExecutionContextGuard::MpsExecutionContextGuard(
    ::orteaf::internal::execution::mps::MpsDeviceHandle device) {
  activate(mps_context::Context{device});
}

MpsExecutionContextGuard::MpsExecutionContextGuard(
    ::orteaf::internal::execution::mps::MpsDeviceHandle device,
    ::orteaf::internal::execution::mps::MpsCommandQueueHandle command_queue) {
  activate(mps_context::Context{device, command_queue});
}

MpsExecutionContextGuard::MpsExecutionContextGuard(
    MpsExecutionContextGuard &&other) noexcept
    : previous_(std::move(other.previous_)), active_(other.active_) {
  other.active_ = false;
}

MpsExecutionContextGuard &MpsExecutionContextGuard::operator=(
    MpsExecutionContextGuard &&other) noexcept {
  if (this != &other) {
    release();
    previous_ = std::move(other.previous_);
    active_ = other.active_;
    other.active_ = false;
  }
  return *this;
}

MpsExecutionContextGuard::~MpsExecutionContextGuard() { release(); }

void MpsExecutionContextGuard::activate(
    ::orteaf::internal::execution_context::mps::Context context) {
  previous_.current = mps_context::currentContext();
  mps_context::setCurrentContext(std::move(context));
  active_ = true;
}

void MpsExecutionContextGuard::release() noexcept {
  if (!active_) {
    return;
  }
  mps_context::setCurrent(std::move(previous_));
  active_ = false;
}

} // namespace orteaf::user::execution_context

#endif // ORTEAF_ENABLE_MPS
