#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "types.h"

// Hard crash on invariant violation — intentional null deref for a clear call stack.
#define ASSERT(cond) do { if (!(cond)) { *(volatile int*)0 = 0; } } while(0)

#define KILOBYTES(n) ((size_t)(n) * 1024)
#define MEGABYTES(n) (KILOBYTES(n) * 1024)

struct Arena
{
    uint8* base;
    size_t size;
    size_t offset;
};

struct AppMemory
{
    void*  block;
    size_t total_size;

    Arena  game_state;
    Arena  renderer;
    Arena  input;
    Arena  test_scratch;
};

static const size_t MEMORY_GAME_STATE_SIZE   = MEGABYTES(16);
static const size_t MEMORY_RENDERER_SIZE     = MEGABYTES(16);
static const size_t MEMORY_INPUT_SIZE        = MEGABYTES(16);
static const size_t MEMORY_TEST_SCRATCH_SIZE = MEGABYTES(16);
static const size_t MEMORY_TOTAL_SIZE        =
    MEMORY_GAME_STATE_SIZE + MEMORY_RENDERER_SIZE +
    MEMORY_INPUT_SIZE      + MEMORY_TEST_SCRATCH_SIZE;

static inline size_t AlignUp(size_t value, size_t align)
{
    ASSERT(align != 0 && (align & (align - 1)) == 0); // align must be a power of two
    return (value + align - 1) & ~(align - 1);
}

static inline void* ArenaPush(Arena* arena, size_t size, size_t align = 8)
{
    size_t aligned_offset = AlignUp(arena->offset, align);
    ASSERT(aligned_offset <= arena->size);
    ASSERT(size <= arena->size - aligned_offset); // overflow-safe bounds check
    void* ptr     = arena->base + aligned_offset;
    arena->offset = aligned_offset + size;
    return ptr;
}

static inline void* ArenaPushArrayHelper(Arena* arena, size_t elem_size, size_t align, size_t count)
{
    ASSERT(count == 0 || elem_size <= (size_t)-1 / count); // guard against size overflow
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
