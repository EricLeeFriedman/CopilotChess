#pragma once

// Runs a single test function and reports pass/fail to the debug output.
// Must be used inside a function that returns bool.
#define RUN_TEST(fn) do { \
    if (!(fn())) { \
        OutputDebugStringA("FAIL: " #fn "\n"); \
        return false; \
    } \
    OutputDebugStringA("PASS: " #fn "\n"); \
} while(0)
