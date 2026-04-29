#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "renderer.h"
#include "moves.h"
#include "input.h"
#include "ui.h"

static AppMemory     g_Memory;
static RendererState g_Renderer;
static GameState*    g_GameState;
static InputState*   g_InputState;
static GameResult    g_GameResult = GAME_ONGOING;

// Board layout constants — shared between the render loop and WindowProc.
static const int32 BOARD_SQUARE_SIZE = 80;
static const int32 BOARD_PX          = BOARD_SQUARE_SIZE * 8; // 640
static const int32 BOARD_X           = (1280 - BOARD_PX) / 2; // 320
static const int32 BOARD_Y           = (720  - BOARD_PX) / 2; // 40

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

    // Adjust so the CLIENT area is exactly 1280×720. CreateWindowEx dimensions
    // include the title bar and borders, so compensate using AdjustWindowRect.
    RECT desired_client = { 0, 0, 1280, 720 };
    AdjustWindowRect(&desired_client, WS_OVERLAPPEDWINDOW, FALSE);
    int32 window_w = (int32)(desired_client.right  - desired_client.left);
    int32 window_h = (int32)(desired_client.bottom - desired_client.top);

    HWND window = CreateWindowEx(
        0,
        "CopilotChess",
        "CopilotChess",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, window_w, window_h,
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

    g_GameState = ArenaPushType(&g_Memory.game_state, GameState);
    InitGameState(g_GameState);

    g_InputState = ArenaPushType(&g_Memory.input, InputState);
    InputInit(g_InputState);

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

        g_GameResult = EvaluatePosition(g_GameState);

        Pixel bg = { 40, 40, 40, 0 };
        ClearBuffer(&g_Renderer, bg);

        if (g_InputState->pending_promotion)
        {
            // Board is drawn normally; the source pawn is hidden so the picker
            // squares stand out on their own.
            DrawBoard(&g_Renderer, g_GameState,
                      BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE,
                      -1, -1, nullptr,
                      g_InputState->promo_from_rank, g_InputState->promo_from_file);

            DrawPromotionPicker(&g_Renderer,
                                BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE,
                                g_InputState->promo_to_rank,
                                g_InputState->promo_to_file,
                                g_GameState->side_to_move);
        }
        else if (g_InputState->dragging)
        {
            int8           from_rank = g_InputState->drag_from_rank;
            int8           from_file = g_InputState->drag_from_file;
            const Square&  dragged   = g_GameState->board.squares[from_rank][from_file];

            // Highlight the source square and show legal-move dots; hide the
            // piece there so it renders only at the cursor (floating).
            DrawBoard(&g_Renderer, g_GameState,
                      BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE,
                      from_rank, from_file,
                      &g_InputState->legal_moves,
                      from_rank, from_file);

            DrawPieceAt(&g_Renderer, dragged.piece, dragged.color,
                        g_InputState->drag_cursor_x, g_InputState->drag_cursor_y,
                        BOARD_SQUARE_SIZE);
        }
        else
        {
            DrawBoard(&g_Renderer, g_GameState,
                      BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE,
                      -1, -1, nullptr);
        }

        if (g_GameResult != GAME_ONGOING)
        {
            DrawStatusOverlay(&g_Renderer, g_GameState, BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE);
            DrawGameOverOverlay(&g_Renderer, g_GameResult,
                                BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE);
        }

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

        case WM_LBUTTONDOWN:
        {
            // LOWORD/HIWORD give unsigned 16-bit values; cast via int16 to
            // correctly sign-extend coordinates that extend off the client area.
            int32 px = (int32)(int16)LOWORD(lparam);
            int32 py = (int32)(int16)HIWORD(lparam);

            if (g_GameResult != GAME_ONGOING)
            {
                // Game over: only the restart button is active.
                if (IsRestartButtonHit(px, py, BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE))
                {
                    ArenaReset(&g_Memory.game_state);
                    ArenaReset(&g_Memory.input);
                    InputRestart(g_InputState, g_GameState);
                }
            }
            else if (g_InputState->pending_promotion)
                InputHandlePromotionClick(g_InputState, g_GameState,
                                          px, py,
                                          BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE);
            else
            {
                InputHandleDragStart(g_InputState, g_GameState,
                                     px, py,
                                     BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE);
                // Capture the mouse so WM_LBUTTONUP is received even if the
                // button is released outside the client area.
                if (g_InputState->dragging)
                    SetCapture(window);
            }
        } return 0;

        case WM_MOUSEMOVE:
        {
            int32 px = (int32)(int16)LOWORD(lparam);
            int32 py = (int32)(int16)HIWORD(lparam);
            InputHandleDragMove(g_InputState, px, py);
        } return 0;

        case WM_LBUTTONUP:
        {
            int32 px = (int32)(int16)LOWORD(lparam);
            int32 py = (int32)(int16)HIWORD(lparam);
            InputHandleDragEnd(g_InputState, g_GameState,
                               px, py,
                               BOARD_X, BOARD_Y, BOARD_SQUARE_SIZE);
            ReleaseCapture();
        } return 0;

        case WM_RBUTTONDOWN:
        {
            InputCancelDrag(g_InputState);
            ReleaseCapture();
        } return 0;

        case WM_CAPTURECHANGED:
        {
            // The OS revoked mouse capture (e.g. Alt+Tab or a modal dialog).
            // Only cancel if a drag is in progress; do NOT cancel when in
            // pending_promotion state, because that state deliberately releases
            // capture (via ReleaseCapture in WM_LBUTTONUP) before the picker
            // click arrives, so WM_CAPTURECHANGED is expected and benign there.
            if (g_InputState->dragging)
                InputCancelDrag(g_InputState);
        } return 0;
    }

    return DefWindowProc(window, message, wparam, lparam);
}

void RunMemoryTests(AppMemory* memory, int32* passed, int32* total);
void RunBoardTests(AppMemory* memory, int32* passed, int32* total);
void RunMovesTests(AppMemory* memory, int32* passed, int32* total);
void RunRendererTests(AppMemory* memory, int32* passed, int32* total);
void RunUITests(AppMemory* memory, int32* passed, int32* total);
void RunInputTests(AppMemory* memory, int32* passed, int32* total);

static bool RunTests(void)
{
    int32 passed = 0;
    int32 total  = 0;

    RunMemoryTests(&g_Memory,   &passed, &total);
    RunBoardTests(&g_Memory,    &passed, &total);
    RunMovesTests(&g_Memory,    &passed, &total);
    RunRendererTests(&g_Memory, &passed, &total);
    RunUITests(&g_Memory,       &passed, &total);
    RunInputTests(&g_Memory,    &passed, &total);

    uint8 summary[64];
    wsprintfA((LPSTR)summary, "%d/%d tests passed\n", passed, total);
    OutputDebugStringA((const char*)summary);

    return passed == total;
}

