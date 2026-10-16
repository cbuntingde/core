#pragma once

// Minimal unit test framework for C++14. No external dependencies.
// Usage:
//   TEST(MyTestName) { ASSERT_EQ(1 + 1, 2); }
// Run all tests from main via RunAllTests().

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <string>
#include <vector>

namespace vmangos_test {

struct TestResult
{
    std::string name;
    bool passed;
    std::string message;
};

struct TestRegistry
{
    std::vector<std::function<void()>> tests;
    std::vector<std::string> names;

    void Register(std::string const& name, std::function<void()> const& fn)
    {
        names.push_back(name);
        tests.push_back(fn);
    }
};

inline TestRegistry& GetRegistry()
{
    static TestRegistry reg;
    return reg;
}

extern int g_failures;
extern int g_assertions;

#define TEST(name) \
    static void name##_fn(); \
    struct name##_registrar { \
        name##_registrar() { \
            vmangos_test::GetRegistry().Register(#name, name##_fn); \
        } \
    }; \
    static name##_registrar name##_reg; \
    static void name##_fn()

#define ASSERT_TRUE(cond) \
    do { \
        ++vmangos_test::g_assertions; \
        if (!(cond)) { \
            std::printf("FAIL: %s:%d ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
            ++vmangos_test::g_failures; \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        ++vmangos_test::g_assertions; \
        if (cond) { \
            std::printf("FAIL: %s:%d ASSERT_FALSE(%s)\n", __FILE__, __LINE__, #cond); \
            ++vmangos_test::g_failures; \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        ++vmangos_test::g_assertions; \
        auto&& _a = (a); \
        auto&& _b = (b); \
        if (!(_a == _b)) { \
            std::printf("FAIL: %s:%d ASSERT_EQ(%s, %s) -- %s != %s\n", \
                __FILE__, __LINE__, #a, #b, \
                vmangos_test::ToString(_a).c_str(), vmangos_test::ToString(_b).c_str()); \
            ++vmangos_test::g_failures; \
            return; \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        ++vmangos_test::g_assertions; \
        auto&& _a = (a); \
        auto&& _b = (b); \
        if (!(_a != _b)) { \
            std::printf("FAIL: %s:%d ASSERT_NE(%s, %s) -- both are %s\n", \
                __FILE__, __LINE__, #a, #b, vmangos_test::ToString(_a).c_str()); \
            ++vmangos_test::g_failures; \
            return; \
        } \
    } while (0)

inline std::string ToString(int v) { return std::to_string(v); }
inline std::string ToString(unsigned v) { return std::to_string(v); }
inline std::string ToString(long v) { return std::to_string(v); }
inline std::string ToString(unsigned long v) { return std::to_string(v); }
inline std::string ToString(long long v) { return std::to_string(v); }
inline std::string ToString(unsigned long long v) { return std::to_string(v); }
inline std::string ToString(float v) { return std::to_string(v); }
inline std::string ToString(double v) { return std::to_string(v); }
inline std::string ToString(bool v) { return v ? "true" : "false"; }
inline std::string ToString(char const* v) { return v ? v : "null"; }
inline std::string ToString(std::string const& v) { return v; }

inline int RunAllTests()
{
    auto& reg = GetRegistry();
    std::printf("Running %u tests...\n", static_cast<unsigned>(reg.tests.size()));
    for (size_t i = 0; i < reg.tests.size(); ++i)
    {
        std::printf("  %s ... ", reg.names[i].c_str());
        std::fflush(stdout);
        reg.tests[i]();
        if (g_failures == 0)
            std::printf("ok\n");
        else
            std::printf("FAILED (%d assertions so far)\n", g_failures);
    }
    std::printf("\n%d assertions, %d failures\n", g_assertions, g_failures);
    return g_failures == 0 ? 0 : 1;
}

} // namespace vmangos_test
