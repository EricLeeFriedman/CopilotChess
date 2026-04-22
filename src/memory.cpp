#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"

bool MemoryInit(AppMemory* memory)
{
    void* block = VirtualAlloc(0, MEMORY_TOTAL_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!block)
        return false;

    memory->block      = block;
    memory->total_size = MEMORY_TOTAL_SIZE;

    uint8* cursor = (uint8*)block;

    memory->game_state   = { cursor, MEMORY_GAME_STATE_SIZE,   0 };
    cursor += MEMORY_GAME_STATE_SIZE;

    memory->renderer     = { cursor, MEMORY_RENDERER_SIZE,     0 };
    cursor += MEMORY_RENDERER_SIZE;

    memory->input        = { cursor, MEMORY_INPUT_SIZE,        0 };
    cursor += MEMORY_INPUT_SIZE;

    memory->test_scratch = { cursor, MEMORY_TEST_SCRATCH_SIZE, 0 };

    return true;
}
