#pragma once

#include <initializer_list>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>

#include "tests/internal/runtime/mps/testing/backend_mock_expectations.h"

namespace orteaf::tests::runtime::mps::testing {

template <class ManagerT, class Provider>
class ManagerAdapter {
public:
    using Manager = ManagerT;
    using Context = typename Provider::Context;

    void bind(Manager& manager, Context& context) {
        manager_ = &manager;
        context_ = &context;
    }

    Manager& manager() {
        return *manager_;
    }

    const Manager& manager() const {
        return *manager_;
    }

    void expectCreateCommandQueues(
        std::initializer_list<::orteaf::internal::backend::mps::MPSCommandQueue_t> handles,
        ::testing::Matcher<::orteaf::internal::backend::mps::MPSDevice_t> matcher = ::testing::_) {
        if constexpr (Provider::is_mock) {
            auto& mock = Provider::mock(*context_);
            BackendMockExpectations::expectCreateCommandQueues(mock, handles, matcher);
        } else {
            (void)handles;
            (void)matcher;
        }
    }

    void expectCreateEvents(
        std::initializer_list<::orteaf::internal::backend::mps::MPSEvent_t> handles,
        ::testing::Matcher<::orteaf::internal::backend::mps::MPSDevice_t> matcher = ::testing::_) {
        if constexpr (Provider::is_mock) {
            auto& mock = Provider::mock(*context_);
            BackendMockExpectations::expectCreateEvents(mock, handles, matcher);
        } else {
            (void)handles;
            (void)matcher;
        }
    }

    void expectDestroyCommandQueues(
        std::initializer_list<::orteaf::internal::backend::mps::MPSCommandQueue_t> handles) {
        if constexpr (Provider::is_mock) {
            auto& mock = Provider::mock(*context_);
            BackendMockExpectations::expectDestroyCommandQueues(mock, handles);
        } else {
            (void)handles;
        }
    }

    void expectDestroyEvents(
        std::initializer_list<::orteaf::internal::backend::mps::MPSEvent_t> handles) {
        if constexpr (Provider::is_mock) {
            auto& mock = Provider::mock(*context_);
            BackendMockExpectations::expectDestroyEvents(mock, handles);
        } else {
            (void)handles;
        }
    }

    void expectDestroyCommandQueuesInOrder(
        std::initializer_list<::orteaf::internal::backend::mps::MPSCommandQueue_t> handles) {
        if constexpr (Provider::is_mock) {
            auto& mock = Provider::mock(*context_);
            BackendMockExpectations::expectDestroyCommandQueuesInOrder(mock, handles);
        } else {
            (void)handles;
        }
    }

    void expectDestroyEventsInOrder(
        std::initializer_list<::orteaf::internal::backend::mps::MPSEvent_t> handles) {
        if constexpr (Provider::is_mock) {
            auto& mock = Provider::mock(*context_);
            BackendMockExpectations::expectDestroyEventsInOrder(mock, handles);
        } else {
            (void)handles;
        }
    }

private:
    Manager* manager_{nullptr};
    Context* context_{nullptr};
};

}  // namespace orteaf::tests::runtime::mps::testing
