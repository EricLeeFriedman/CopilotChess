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

    int* a = ArenaPushType(arena, int);
    if (!a) return false;
    *a = 42;
    if (*a != 42) return false;
    if (arena->offset < sizeof(int)) return false;

    return true;
}

static bool TestArena_NoPushOverlap(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    int* a = ArenaPushType(arena, int);
    int* b = ArenaPushType(arena, int);
    if (!a || !b) return false;
    if (b <= a) return false;

    return true;
}

static bool TestArena_AlignmentPadding(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    // Push a single byte so offset == 1, then push an int — must be naturally aligned.
    char* c = ArenaPushType(arena, char);
    (void)c;
    int* d = ArenaPushType(arena, int);
    if (!d) return false;
    if ((size_t)d % alignof(int) != 0) return false;

    return true;
}

static bool TestArena_ArrayPush(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    int* arr = ArenaPushArray(arena, int, 10);
    if (!arr) return false;
    for (int i = 0; i < 10; ++i) arr[i] = i;
    for (int i = 0; i < 10; ++i) if (arr[i] != i) return false;

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

    ArenaPushType(arena, int);
    if (arena->offset == 0) return false;
    ArenaReset(arena);
    if (arena->offset != 0) return false;

    return true;
}

// Verifies overflow detection without triggering the assertion.
// Fills the arena to within one int of capacity, then confirms:
//   - the arithmetic that guards ArenaPush correctly identifies the would-be overflow
//   - a push at the exact boundary still succeeds
static bool TestArena_OverflowDetectable(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    // Fill all but sizeof(int) bytes using alignment 1 to avoid padding surprises.
    size_t leave = sizeof(int);
    ArenaPush(arena, arena->size - leave, 1);

    // Exactly one int's worth of space should remain.
    size_t remaining = arena->size - arena->offset;
    if (remaining != leave) return false;

    // Two ints would overflow — verify the pre-flight arithmetic catches it.
    if (arena->offset + sizeof(int) * 2 <= arena->size) return false;

    // One int fits — the same arithmetic should pass.
    if (arena->offset + sizeof(int) > arena->size) return false;

    // Confirm the actual push succeeds and exhausts the arena.
    int* p = ArenaPushType(arena, int);
    if (!p) return false;
    if (arena->offset != arena->size) return false;

    return true;
}

static bool TestMemory_ArenasCoverFullBlock(void)
{
    size_t covered =
        s_Memory->game_state.size +
        s_Memory->renderer.size   +
        s_Memory->input.size      +
        s_Memory->test_scratch.size;
    if (covered != MEMORY_TOTAL_SIZE)  return false;
    if (covered != s_Memory->total_size) return false;

    return true;
}

static bool TestMemory_ArenasAreContiguous(void)
{
    unsigned char* base = (unsigned char*)s_Memory->block;

    if (s_Memory->game_state.base   != base)                                   return false;
    if (s_Memory->renderer.base     != base + MEMORY_GAME_STATE_SIZE)          return false;
    if (s_Memory->input.base        != base + MEMORY_GAME_STATE_SIZE
                                            + MEMORY_RENDERER_SIZE)            return false;
    if (s_Memory->test_scratch.base != base + MEMORY_GAME_STATE_SIZE
                                            + MEMORY_RENDERER_SIZE
                                            + MEMORY_INPUT_SIZE)               return false;
    return true;
}

bool RunMemoryTests(AppMemory* memory)
{
    s_Memory = memory;

    RUN_TEST(TestArena_BasicPush);
    RUN_TEST(TestArena_NoPushOverlap);
    RUN_TEST(TestArena_AlignmentPadding);
    RUN_TEST(TestArena_ArrayPush);
    RUN_TEST(TestArena_PushToExactBoundary);
    RUN_TEST(TestArena_ResetRestoresOffset);
    RUN_TEST(TestArena_OverflowDetectable);
    RUN_TEST(TestMemory_ArenasCoverFullBlock);
    RUN_TEST(TestMemory_ArenasAreContiguous);

    return true;
}
