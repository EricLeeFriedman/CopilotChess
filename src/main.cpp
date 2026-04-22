#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"

static AppMemory g_Memory;

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
static bool RunTests(void);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    UNREFERENCED_PARAMETER(prevInstance);

    if (!MemoryInit(&g_Memory))
    {
        MessageBoxA(0, "Failed to allocate application memory.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (lstrcmpA(commandLine, "--test") == 0)
    {
        bool passed = RunTests();
        return passed ? 0 : 1;
    }

    WNDCLASSEX windowClass    = {};
    windowClass.cbSize        = sizeof(windowClass);
    windowClass.style         = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = WindowProc;
    windowClass.hInstance     = instance;
    windowClass.hCursor       = LoadCursor(0, IDC_ARROW);
    windowClass.lpszClassName = "CopilotChess";

    if (!RegisterClassEx(&windowClass))
    {
        MessageBoxA(0, "Failed to register window class.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND window = CreateWindowEx(
        0,
        "CopilotChess",
        "CopilotChess",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        0, 0, instance, 0
    );

    if (!window)
    {
        MessageBoxA(0, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(window, showCode);

    bool running = true;
    while (running)
    {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                running = false;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // TODO: update and render
    }

    return 0;
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } return 0;
    }

    return DefWindowProc(window, message, wparam, lparam);
}

// ---- Arena tests -------------------------------------------------------

static bool TestArena_BasicPush(void)
{
    Arena* arena = &g_Memory.test_scratch;
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
    Arena* arena = &g_Memory.test_scratch;
    ArenaReset(arena);

    int* a = ArenaPushType(arena, int);
    int* b = ArenaPushType(arena, int);
    if (!a || !b) return false;
    if (b <= a) return false;

    return true;
}

static bool TestArena_AlignmentPadding(void)
{
    Arena* arena = &g_Memory.test_scratch;
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
    Arena* arena = &g_Memory.test_scratch;
    ArenaReset(arena);

    int* arr = ArenaPushArray(arena, int, 10);
    if (!arr) return false;
    for (int i = 0; i < 10; ++i) arr[i] = i;
    for (int i = 0; i < 10; ++i) if (arr[i] != i) return false;

    return true;
}

static bool TestArena_PushToExactBoundary(void)
{
    Arena* arena = &g_Memory.test_scratch;
    ArenaReset(arena);

    void* full = ArenaPush(arena, arena->size, 1);
    if (!full) return false;
    if (arena->offset != arena->size) return false;

    return true;
}

static bool TestArena_ResetRestoresOffset(void)
{
    Arena* arena = &g_Memory.test_scratch;
    ArenaReset(arena);

    ArenaPushType(arena, int);
    if (arena->offset == 0) return false;
    ArenaReset(arena);
    if (arena->offset != 0) return false;

    return true;
}

static bool TestMemory_ArenasCoverFullBlock(void)
{
    size_t covered =
        g_Memory.game_state.size +
        g_Memory.renderer.size   +
        g_Memory.input.size      +
        g_Memory.test_scratch.size;
    if (covered != MEMORY_TOTAL_SIZE)      return false;
    if (covered != g_Memory.total_size)    return false;

    return true;
}

static bool TestMemory_ArenasAreContiguous(void)
{
    unsigned char* base = (unsigned char*)g_Memory.block;

    if (g_Memory.game_state.base   != base)                                     return false;
    if (g_Memory.renderer.base     != base + MEMORY_GAME_STATE_SIZE)            return false;
    if (g_Memory.input.base        != base + MEMORY_GAME_STATE_SIZE
                                           + MEMORY_RENDERER_SIZE)              return false;
    if (g_Memory.test_scratch.base != base + MEMORY_GAME_STATE_SIZE
                                           + MEMORY_RENDERER_SIZE
                                           + MEMORY_INPUT_SIZE)                 return false;
    return true;
}

// ---- Test runner --------------------------------------------------------

#define RUN_TEST(fn) do { \
    if (!(fn())) { \
        OutputDebugStringA("FAIL: " #fn "\n"); \
        return false; \
    } \
    OutputDebugStringA("PASS: " #fn "\n"); \
} while(0)

static bool RunTests(void)
{
    RUN_TEST(TestArena_BasicPush);
    RUN_TEST(TestArena_NoPushOverlap);
    RUN_TEST(TestArena_AlignmentPadding);
    RUN_TEST(TestArena_ArrayPush);
    RUN_TEST(TestArena_PushToExactBoundary);
    RUN_TEST(TestArena_ResetRestoresOffset);
    RUN_TEST(TestMemory_ArenasCoverFullBlock);
    RUN_TEST(TestMemory_ArenasAreContiguous);
    return true;
}
