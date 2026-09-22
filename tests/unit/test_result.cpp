#include "../../contracts/common/result.hpp"
#include <cassert>
#include <iostream>

void test_result_success() {
    vani::contracts::Result<int> res(42);
    assert(res.is_ok());
    assert(!res.is_err());
    assert(res.value() == 42);
    assert(res.value_or(0) == 42);
    std::cout << "  [PASS] test_result_success\n";
}

void test_result_error() {
    auto err = vani::contracts::Error::make(
        vani::contracts::ErrorCode::PermissionDenied,
        "User denied execution",
        "vani.policy"
    );
    vani::contracts::Result<int> res(err);
    assert(!res.is_ok());
    assert(res.is_err());
    assert(res.error().code == vani::contracts::ErrorCode::PermissionDenied);
    assert(res.value_or(100) == 100);
    std::cout << "  [PASS] test_result_error\n";
}

void test_result_void() {
    auto ok_res = vani::contracts::Result<void>::ok();
    assert(ok_res.is_ok());

    auto err_res = vani::contracts::Result<void>::err(
        vani::contracts::ErrorCode::Timeout,
        "Network timed out"
    );
    assert(err_res.is_err());
    assert(err_res.error().code == vani::contracts::ErrorCode::Timeout);
    std::cout << "  [PASS] test_result_void\n";
}
