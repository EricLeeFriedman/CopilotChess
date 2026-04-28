#pragma once

#include "types.h"

// A single test case: a human-readable name and a zero-argument function
// that returns true on pass and false on fail.
struct TestEntry {
    const uint8* name;
    bool (*fn)(void);
};

// Convenience macro — avoids writing the name string and the function pointer
// separately.  Usage: TEST_ENTRY(MyTestFunction)
#define TEST_ENTRY(fn) { (const uint8*)#fn, fn }

// Iterates every entry in `tests[0..count)`, calls each function, prints
// "PASS: <name>" or "FAIL: <name>" to the debug output, and accumulates
// results into *passed and *total.  All tests are always executed; nothing
// short-circuits on failure.
inline void RunTestArray(const TestEntry* tests, uint64 count,
                         int32* passed, int32* total)
{
    for (uint64 i = 0; i < count; ++i)
    {
        bool ok = tests[i].fn();
        OutputDebugStringA(ok ? "PASS: " : "FAIL: ");
        OutputDebugStringA((const char*)tests[i].name);
        OutputDebugStringA("\n");
        if (ok) ++(*passed);
        ++(*total);
    }
}
