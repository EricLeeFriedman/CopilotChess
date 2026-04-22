#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "types.h"

// Hard crash on invariant violation — intentional null deref for a clear call stack.
#define ASSERT(cond) do { if (!(cond)) { *(volatile int*)0 = 0; } } while(0)

#define KILOBYTES(n) ((uint64)(n) * 1024)
#define MEGABYTES(n) (KILOBYTES(n) * 1024)

struct Arena
{
    uint8*  base;
    uint64  size;
    uint64  offset;
};

struct AppMemory
{
    void*  block;
    uint64 total_size;

    Arena  game_state;
    Arena  renderer;
    Arena  input;
    Arena  test_scratch;
};

static const uint64 MEMORY_GAME_STATE_SIZE   = MEGABYTES(16);
static const uint64 MEMORY_RENDERER_SIZE     = MEGABYTES(16);
static const uint64 MEMORY_INPUT_SIZE        = MEGABYTES(16);
static const uint64 MEMORY_TEST_SCRATCH_SIZE = MEGABYTES(16);
static const uint64 MEMORY_TOTAL_SIZE        =
    MEMORY_GAME_STATE_SIZE + MEMORY_RENDERER_SIZE +
    MEMORY_INPUT_SIZE      + MEMORY_TEST_SCRATCH_SIZE;

static inline uint64 AlignUp(uint64 value, uint64 align)
{
    ASSERT(align != 0 && (align & (align - 1)) == 0); // align must be a power of two
    return (value + align - 1) & ~(align - 1);
}

static inline void* ArenaPush(Arena* arena, uint64 size, uint64 align = 8)
{
    uint64 aligned_offset = AlignUp(arena->offset, align);
    ASSERT(aligned_offset <= arena->size);
    ASSERT(size <= arena->size - aligned_offset); // overflow-safe bounds check
    void* ptr     = arena->base + aligned_offset;
    arena->offset = aligned_offset + size;
    return ptr;
}

static inline void* ArenaPushArrayHelper(Arena* arena, uint64 elem_size, uint64 align, uint64 count)
{
    ASSERT(count == 0 || elem_size <= ~(uint64)0 / count); // guard against size overflow
    return ArenaPush(arena, elem_size * count, align);
}

#define ArenaPushType(arena, T) \
    ((T*)ArenaPush((arena), sizeof(T), alignof(T)))

#define ArenaPushArray(arena, T, n) \
    ((T*)ArenaPushArrayHelper((arena), sizeof(T), alignof(T), (n)))

static inline void ArenaReset(Arena* arena)
{
    arena->offset = 0;
}

bool MemoryInit(AppMemory* memory);
