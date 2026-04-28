#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "renderer.h"

static AppMemory    g_Memory;
static RendererState g_Renderer;

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

    if (!RendererInit(&g_Renderer, &g_Memory.renderer, 1280, 720))
    {
        MessageBoxA(0, "Failed to initialize renderer.", "Error", MB_OK | MB_ICONERROR);
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

        if (!running) break;

        Pixel bg = { 40, 40, 40, 0 };
        ClearBuffer(&g_Renderer, bg);
        PresentFrame(&g_Renderer, window);
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

void RunMemoryTests(AppMemory* memory, int32* passed, int32* total);
void RunBoardTests(AppMemory* memory, int32* passed, int32* total);
void RunMovesTests(AppMemory* memory, int32* passed, int32* total);
void RunRendererTests(AppMemory* memory, int32* passed, int32* total);

static bool RunTests(void)
{
    int32 passed = 0;
    int32 total  = 0;

    RunMemoryTests(&g_Memory,   &passed, &total);
    RunBoardTests(&g_Memory,    &passed, &total);
    RunMovesTests(&g_Memory,    &passed, &total);
    RunRendererTests(&g_Memory, &passed, &total);

    uint8 summary[64];
    wsprintfA((LPSTR)summary, "%d/%d tests passed\n", passed, total);
    OutputDebugStringA((const char*)summary);

    return passed == total;
}

