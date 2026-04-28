#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "tests.h"

// Set by RunMemoryTests before the suite runs; all test functions read from it.
static AppMemory* s_Memory;

static bool TestArena_BasicPush(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    int32* a = ArenaPushType(arena, int32);
    if (!a) return false;
    *a = 42;
    if (*a != 42) return false;
    if (arena->offset < sizeof(int32)) return false;

    return true;
}

static bool TestArena_NoPushOverlap(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    int32* a = ArenaPushType(arena, int32);
    int32* b = ArenaPushType(arena, int32);
    if (!a || !b) return false;
    if (b <= a) return false;

    return true;
}

static bool TestArena_AlignmentPadding(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    // Push a single byte so offset == 1, then push an int32 — must be naturally aligned.
    uint8* c = ArenaPushType(arena, uint8);
    (void)c;
    int32* d = ArenaPushType(arena, int32);
    if (!d) return false;
    if ((uint64)d % alignof(int32) != 0) return false;

    return true;
}

static bool TestArena_ArrayPush(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    int32* arr = ArenaPushArray(arena, int32, 10);
    if (!arr) return false;
    for (int32 i = 0; i < 10; ++i) arr[i] = i;
    for (int32 i = 0; i < 10; ++i) if (arr[i] != i) return false;

    return true;
}

static bool TestArena_PushToExactBoundary(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    void* full = ArenaPush(arena, arena->size, 1);
    if (!full) return false;
    if (arena->offset != arena->size) return false;

    return true;
}

static bool TestArena_ResetRestoresOffset(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    ArenaPushType(arena, int32);
    if (arena->offset == 0) return false;
    ArenaReset(arena);
    if (arena->offset != 0) return false;

    return true;
}

// Verifies overflow detection without triggering the assertion.
// Fills the arena to within one int32 of capacity, then confirms:
//   - the arithmetic that guards ArenaPush correctly identifies the would-be overflow
//   - a push at the exact boundary still succeeds
static bool TestArena_OverflowDetectable(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    // Fill all but sizeof(int32) bytes using alignment 1 to avoid padding surprises.
    uint64 leave = sizeof(int32);
    ArenaPush(arena, arena->size - leave, 1);

    // Exactly one int32's worth of space should remain.
    uint64 remaining = arena->size - arena->offset;
    if (remaining != leave) return false;

    // Two int32s would overflow — verify the pre-flight arithmetic catches it.
    if (arena->offset + sizeof(int32) * 2 <= arena->size) return false;

    // One int32 fits — the same arithmetic should pass.
    if (arena->offset + sizeof(int32) > arena->size) return false;

    // Confirm the actual push succeeds and exhausts the arena.
    int32* p = ArenaPushType(arena, int32);
    if (!p) return false;
    if (arena->offset != arena->size) return false;

    return true;
}

static bool TestMemory_ArenasCoverFullBlock(void)
{
    uint64 covered =
        s_Memory->game_state.size +
        s_Memory->renderer.size   +
        s_Memory->input.size      +
        s_Memory->test_scratch.size;
    if (covered != MEMORY_TOTAL_SIZE)    return false;
    if (covered != s_Memory->total_size) return false;

    return true;
}

static bool TestMemory_ArenasAreContiguous(void)
{
    uint8* base = (uint8*)s_Memory->block;

    if (s_Memory->game_state.base   != base)                                   return false;
    if (s_Memory->renderer.base     != base + MEMORY_GAME_STATE_SIZE)          return false;
    if (s_Memory->input.base        != base + MEMORY_GAME_STATE_SIZE
                                            + MEMORY_RENDERER_SIZE)            return false;
    if (s_Memory->test_scratch.base != base + MEMORY_GAME_STATE_SIZE
                                            + MEMORY_RENDERER_SIZE
                                            + MEMORY_INPUT_SIZE)               return false;
    return true;
}

static const TestEntry k_MemoryTests[] = {
    TEST_ENTRY(TestArena_BasicPush),
    TEST_ENTRY(TestArena_NoPushOverlap),
    TEST_ENTRY(TestArena_AlignmentPadding),
    TEST_ENTRY(TestArena_ArrayPush),
    TEST_ENTRY(TestArena_PushToExactBoundary),
    TEST_ENTRY(TestArena_ResetRestoresOffset),
    TEST_ENTRY(TestArena_OverflowDetectable),
    TEST_ENTRY(TestMemory_ArenasCoverFullBlock),
    TEST_ENTRY(TestMemory_ArenasAreContiguous),
};

void RunMemoryTests(AppMemory* memory, int32* passed, int32* total)
{
    s_Memory = memory;
    RunTestArray(k_MemoryTests, sizeof(k_MemoryTests) / sizeof(k_MemoryTests[0]),
                 passed, total);
}

