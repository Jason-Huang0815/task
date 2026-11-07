#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace test {

struct Case {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline void reportFailure(const char* file, int line, const char* expression) {
    ++failures();
    std::printf("    [FAIL] %s:%d  %s\n", file, line, expression);
}

inline int runAll() {
    int failedCases = 0;
    for (auto& item : registry()) {
        failures() = 0;
        std::printf("[ RUN  ] %s\n", item.name);
        item.fn();
        if (failures() == 0) {
            std::printf("[  OK  ] %s\n", item.name);
        } else {
            ++failedCases;
            std::printf("[ FAIL ] %s (%d failed checks)\n", item.name, failures());
        }
    }
    std::printf("\n%d cases, %d failed\n", static_cast<int>(registry().size()), failedCases);
    return failedCases == 0 ? 0 : 1;
}

}  // namespace test

#define TEST_CASE(name)                                        \
    static void name();                                        \
    static ::test::Registrar registrar_##name(#name, name);    \
    static void name()

#define CHECK(expr)                                            \
    do {                                                       \
        if (!(expr)) {                                         \
            ::test::reportFailure(__FILE__, __LINE__, #expr);  \
        }                                                      \
    } while (0)

#define CHECK_EQ(a, b)                                              \
    do {                                                            \
        if (!((a) == (b))) {                                        \
            ::test::reportFailure(__FILE__, __LINE__, #a " == " #b); \
        }                                                           \
    } while (0)
