#include "orteaf/internal/diagnostics/error/exception.hpp"

#include <gtest/gtest.h>

namespace diagnostics_error = orteaf::internal::diagnostics::error;

TEST(DiagnosticsError, ThrowRuntimeErrorRaisesStdRuntimeError) {
    constexpr const char* kMessage = "diagnostics runtime failure";
    try {
        diagnostics_error::throwRuntimeError(kMessage);
        FAIL() << "Expected std::runtime_error to be thrown";
    } catch (const std::runtime_error& ex) {
        EXPECT_STREQ(kMessage, ex.what());
    } catch (...) {
        FAIL() << "Unexpected exception type thrown";
    }
}

TEST(DiagnosticsError, WrapAndRethrowReturnsWrappedValue) {
    const auto result = diagnostics_error::wrapAndRethrow([] { return 42; });
    EXPECT_EQ(42, result);
}

TEST(DiagnosticsError, WrapAndRethrowPropagatesExceptions) {
    EXPECT_THROW(
        diagnostics_error::wrapAndRethrow(
            []() -> void { throw std::logic_error("logical failure"); }),
        std::logic_error);
}
