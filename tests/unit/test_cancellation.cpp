#include "../../contracts/common/cancellation_token.hpp"
#include <cassert>
#include <iostream>

void test_cancellation_flow() {
    vani::contracts::CancellationSource source;
    auto token = source.token();

    assert(!token.is_cancelled());
    assert(!source.is_cancelled());

    bool callback_called = false;
    token.register_callback([&]() {
        callback_called = true;
    });

    source.cancel();

    assert(token.is_cancelled());
    assert(source.is_cancelled());
    assert(callback_called);

    std::cout << "  [PASS] test_cancellation_flow\n";
}
