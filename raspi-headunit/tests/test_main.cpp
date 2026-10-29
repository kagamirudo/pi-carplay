#include "test_harness.hpp"

#include <iostream>

void testConfig();
void testStorage();
void testPhoneConnection();

int main() {
    testConfig();
    testStorage();
    testPhoneConnection();
    if (headunit::test::g_failed != 0) {
        std::cerr << headunit::test::g_failed << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}
