#include "TestFramework.h"

int vmangos_test::g_failures = 0;
int vmangos_test::g_assertions = 0;

char const* g_mainLogFileName = "vmangos_tests.log";

int main()
{
    return vmangos_test::RunAllTests();
}
