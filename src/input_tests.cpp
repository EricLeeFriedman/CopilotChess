#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "moves.h"
#include "input.h"
#include "tests.h"

static AppMemory* s_Memory;

// ---------------------------------------------------------------------------
// PixelToSquare tests
// ---------------------------------------------------------------------------

// Pixel exactly at the top-left corner of a1 (rank 0, file 0) with a
// board_x=320, board_y=40, square_size=80 layout.
// a1 is rank 0 / file 0; its top-left screen pixel is
// (320 + 0*80, 40 + (7-0)*80) = (320, 600).
static bool TestInput_PixelToSquare_A1_Corner(void)
{
    int8 rank = 99, file = 99;
    bool ok = PixelToSquare(320, 600, 320, 40, 80, &rank, &file);
    if (!ok)           return false;
    if (rank != 0)     return false;
    if (file != 0)     return false;
    return true;
}

// Pixel at the center of h8 (rank 7, file 7).
// sq_x = 320 + 7*80 = 880; sq_y = 40 + (7-7)*80 = 40.
// Center: (880+40, 40+40) = (920, 80).
static bool TestInput_PixelToSquare_H8_Center(void)
{
    int8 rank = 99, file = 99;
    bool ok = PixelToSquare(920, 80, 320, 40, 80, &rank, &file);
    if (!ok)           return false;
    if (rank != 7)     return false;
    if (file != 7)     return false;
    return true;
}

// Pixel one pixel to the left of the board should return false.
static bool TestInput_PixelToSquare_OutsideLeft(void)
{
    int8 rank = 0, file = 0;
    bool ok = PixelToSquare(319, 300, 320, 40, 80, &rank, &file);
    return !ok;
}

// Pixel one pixel below the board (board ends at 40 + 640 = 680) should
// return false.
static bool TestInput_PixelToSquare_OutsideBottom(void)
{
    int8 rank = 0, file = 0;
    bool ok = PixelToSquare(400, 680, 320, 40, 80, &rank, &file);
    return !ok;
}

// Pixel at the very last valid bottom-right corner (h1: rank 0, file 7).
// sq_x = 320 + 7*80 = 880; sq_y = 40 + 7*80 = 600.
// Last valid pixel inside: (880 + 79, 600 + 79) = (959, 679).
static bool TestInput_PixelToSquare_H1_LastPixel(void)
{
    int8 rank = 99, file = 99;
    bool ok = PixelToSquare(959, 679, 320, 40, 80, &rank, &file);
    if (!ok)           return false;
    if (rank != 0)     return false;
    if (file != 7)     return false;
    return true;
}

// ---------------------------------------------------------------------------
// InputInit / InputCancelDrag tests
// ---------------------------------------------------------------------------

static bool TestInput_InitClearsState(void)
{
    InputState input;
    // Dirty the struct first.
    input.dragging       = true;
    input.drag_from_rank = 3;
    input.drag_from_file = 4;
    input.drag_cursor_x  = 500;
    input.drag_cursor_y  = 400;

    InputInit(&input);

    if (input.dragging)              return false;
    if (input.drag_from_rank != -1)  return false;
    if (input.drag_from_file != -1)  return false;
    if (input.legal_moves.count != 0) return false;
    return true;
}

static bool TestInput_CancelDragResetsState(void)
{
    InputState input = {};
    input.dragging       = true;
    input.drag_from_rank = 2;
    input.drag_from_file = 3;
    input.legal_moves.count = 5; // fake count

    InputCancelDrag(&input);

    if (input.dragging)               return false;
    if (input.drag_from_rank != -1)   return false;
    if (input.drag_from_file != -1)   return false;
    if (input.legal_moves.count != 0) return false;
    return true;
}

// ---------------------------------------------------------------------------
// InputHandleDragStart tests
// ---------------------------------------------------------------------------

// Clicking on a White pawn at e2 (rank 1, file 4) should start a drag and
// populate legal_moves.
// Board layout: board_x=320, board_y=40, square_size=80.
// e2 screen center: sq_x=320+4*80=640, sq_y=40+(7-1)*80=520; center=(680,560).
static bool TestInput_DragStart_PicksUpOwnPiece(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs); // White to move

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80);

    if (!input.dragging)              return false;
    if (input.drag_from_rank != 1)    return false;
    if (input.drag_from_file != 4)    return false;
    if (input.legal_moves.count == 0) return false; // e2-e3 and e2-e4 at minimum
    return true;
}

// Clicking on a Black piece (rank 6, file 4 = e7) with White to move should
// NOT start a drag.
// e7 center: sq_x=320+4*80=640, sq_y=40+(7-6)*80=120; center=(680,160).
static bool TestInput_DragStart_IgnoresOpponentPiece(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs); // White to move

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);

    if (input.dragging) return false;
    return true;
}

// Clicking on an empty square should NOT start a drag.
// e4 (rank 3, file 4) is empty at game start.
// e4 center: sq_x=320+4*80=640, sq_y=40+(7-3)*80=360; center=(680,400).
static bool TestInput_DragStart_IgnoresEmptySquare(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 400, 320, 40, 80);

    if (input.dragging) return false;
    return true;
}

// Clicking outside the board entirely should NOT start a drag.
static bool TestInput_DragStart_IgnoresOutsideBoard(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 10, 10, 320, 40, 80);

    if (input.dragging) return false;
    return true;
}

// ---------------------------------------------------------------------------
// InputHandleDragMove tests
// ---------------------------------------------------------------------------

// After starting a drag, DragMove should update the cursor coordinates.
static bool TestInput_DragMove_UpdatesCursor(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80); // pick up e2
    InputHandleDragMove(&input, 700, 400);

    if (!input.dragging)           return false;
    if (input.drag_cursor_x != 700) return false;
    if (input.drag_cursor_y != 400) return false;
    return true;
}

// DragMove while not dragging is a safe no-op.
static bool TestInput_DragMove_NoOpWhenNotDragging(void)
{
    InputState input = {};
    InputInit(&input);

    InputHandleDragMove(&input, 500, 500); // must not crash or set dragging

    if (input.dragging) return false;
    return true;
}

// ---------------------------------------------------------------------------
// InputHandleDragEnd tests
// ---------------------------------------------------------------------------

// Drop on a legal target (e4) after picking up e2 should apply the move.
// e4: sq_y=40+(7-3)*80=360, center=(680,400).
static bool TestInput_DragEnd_AppliesLegalMove(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs); // White to move

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80); // pick up e2
    bool moved = InputHandleDragEnd(&input, &gs, 680, 400, 320, 40, 80); // drop on e4

    if (!moved)                                        return false;
    if (input.dragging)                                return false; // cleared after move
    if (gs.board.squares[3][4].piece != PIECE_PAWN)   return false; // pawn on e4
    if (gs.board.squares[3][4].color != COLOR_WHITE)   return false;
    if (gs.board.squares[1][4].piece != PIECE_NONE)   return false; // e2 is empty
    if (gs.side_to_move != COLOR_BLACK)                return false; // turn advanced
    return true;
}

// Drop on an illegal target (e5) cancels the drag without changing the board.
// e5 (rank 4): sq_y=40+(7-4)*80=280, center=(680,320).
static bool TestInput_DragEnd_CancelsOnIllegalTarget(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80); // pick up e2
    bool moved = InputHandleDragEnd(&input, &gs, 680, 320, 320, 40, 80); // drop on e5

    if (moved)                                        return false;
    if (input.dragging)                               return false; // drag cancelled
    if (gs.board.squares[1][4].piece != PIECE_PAWN)  return false; // board unchanged
    if (gs.side_to_move != COLOR_WHITE)               return false; // turn unchanged
    return true;
}

// Drop outside the board cancels the drag.
static bool TestInput_DragEnd_CancelsOutsideBoard(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80); // pick up e2
    bool moved = InputHandleDragEnd(&input, &gs, 10, 10, 320, 40, 80); // outside board

    if (moved)          return false;
    if (input.dragging) return false;
    return true;
}

// DragEnd when not dragging is a safe no-op returning false.
static bool TestInput_DragEnd_NoOpWhenNotDragging(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    bool moved = InputHandleDragEnd(&input, &gs, 680, 400, 320, 40, 80);

    if (moved)          return false;
    if (input.dragging) return false;
    return true;
}

// InputCancelDrag during an active drag clears drag state.
static bool TestInput_CancelDrag_ClearsActiveDrag(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80); // pick up e2
    if (!input.dragging) return false;

    InputCancelDrag(&input);

    if (input.dragging)               return false;
    if (input.drag_from_rank != -1)   return false;
    if (input.legal_moves.count != 0) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Test table and entry point
// ---------------------------------------------------------------------------

static const TestEntry k_InputTests[] = {
    TEST_ENTRY(TestInput_PixelToSquare_A1_Corner),
    TEST_ENTRY(TestInput_PixelToSquare_H8_Center),
    TEST_ENTRY(TestInput_PixelToSquare_OutsideLeft),
    TEST_ENTRY(TestInput_PixelToSquare_OutsideBottom),
    TEST_ENTRY(TestInput_PixelToSquare_H1_LastPixel),
    TEST_ENTRY(TestInput_InitClearsState),
    TEST_ENTRY(TestInput_CancelDragResetsState),
    TEST_ENTRY(TestInput_DragStart_PicksUpOwnPiece),
    TEST_ENTRY(TestInput_DragStart_IgnoresOpponentPiece),
    TEST_ENTRY(TestInput_DragStart_IgnoresEmptySquare),
    TEST_ENTRY(TestInput_DragStart_IgnoresOutsideBoard),
    TEST_ENTRY(TestInput_DragMove_UpdatesCursor),
    TEST_ENTRY(TestInput_DragMove_NoOpWhenNotDragging),
    TEST_ENTRY(TestInput_DragEnd_AppliesLegalMove),
    TEST_ENTRY(TestInput_DragEnd_CancelsOnIllegalTarget),
    TEST_ENTRY(TestInput_DragEnd_CancelsOutsideBoard),
    TEST_ENTRY(TestInput_DragEnd_NoOpWhenNotDragging),
    TEST_ENTRY(TestInput_CancelDrag_ClearsActiveDrag),
};

void RunInputTests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_InputTests, sizeof(k_InputTests) / sizeof(k_InputTests[0]),
                 passed, total);
}
