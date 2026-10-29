#pragma once

#include <iostream>

namespace headunit::test {

inline int g_failed = 0;

inline void expect(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        std::cerr << "FAIL " << file << ":" << line << " " << expression << '\n';
        ++g_failed;
    }
}

}  // namespace headunit::test

#define EXPECT(condition) ::headunit::test::expect(static_cast<bool>(condition), #condition, __FILE__, __LINE__)
