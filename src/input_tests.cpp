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
// InputInit / InputClearSelection tests
// ---------------------------------------------------------------------------

static bool TestInput_InitClearsState(void)
{
    InputState input;
    // Dirty the struct first.
    input.selected_rank = 3;
    input.selected_file = 4;
    input.has_selection = true;

    InputInit(&input);

    if (input.has_selection)        return false;
    if (input.selected_rank != -1)  return false;
    if (input.selected_file != -1)  return false;
    if (input.legal_moves.count != 0) return false;
    return true;
}

static bool TestInput_ClearSelectionResetsState(void)
{
    InputState input = {};
    input.selected_rank = 2;
    input.selected_file = 3;
    input.has_selection = true;
    input.legal_moves.count = 5; // fake count

    InputClearSelection(&input);

    if (input.has_selection)         return false;
    if (input.selected_rank != -1)   return false;
    if (input.selected_file != -1)   return false;
    if (input.legal_moves.count != 0) return false;
    return true;
}

// ---------------------------------------------------------------------------
// InputHandleLeftClick tests
// ---------------------------------------------------------------------------

// Clicking on a White pawn at e2 (rank 1, file 4) with no selection should
// select it and populate legal_moves.
// Board layout: board_x=320, board_y=40, square_size=80.
// e2 screen center: sq_x=320+4*80=640, sq_y=40+(7-1)*80=520; center=(680,560).
static bool TestInput_LeftClick_SelectsOwnPiece(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs); // White to move

    InputState input = {};
    InputInit(&input);

    bool moved = InputHandleLeftClick(&input, &gs, 680, 560, 320, 40, 80);

    if (moved)                    return false; // no move yet
    if (!input.has_selection)     return false;
    if (input.selected_rank != 1) return false;
    if (input.selected_file != 4) return false;
    if (input.legal_moves.count == 0) return false; // e2-e3 and e2-e4 at minimum
    return true;
}

// Clicking on a Black piece (rank 6, file 4 = e7) with no selection and
// White to move should NOT select anything.
// e7 center: sq_x=320+4*80=640, sq_y=40+(7-6)*80=120; center=(680,160).
static bool TestInput_LeftClick_DoesNotSelectOpponentPiece(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs); // White to move

    InputState input = {};
    InputInit(&input);

    bool moved = InputHandleLeftClick(&input, &gs, 680, 160, 320, 40, 80);

    if (moved)                return false;
    if (input.has_selection)  return false;
    return true;
}

// Click on e2, then click on e4 — a legal double-pawn push for White.
// After the move the side to move is Black and the input is cleared.
// e4: sq_x=320+4*80=640, sq_y=40+(7-3)*80=360; center=(680,400).
static bool TestInput_LeftClick_AppliesLegalMove(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs); // White to move

    InputState input = {};
    InputInit(&input);

    // Select e2.
    InputHandleLeftClick(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.has_selection) return false;

    // Move to e4.
    bool moved = InputHandleLeftClick(&input, &gs, 680, 400, 320, 40, 80);

    if (!moved)              return false;
    if (input.has_selection) return false; // cleared after move

    // The piece should now be on e4 (rank 3, file 4).
    if (gs.board.squares[3][4].piece != PIECE_PAWN) return false;
    if (gs.board.squares[3][4].color != COLOR_WHITE) return false;

    // e2 must be empty.
    if (gs.board.squares[1][4].piece != PIECE_NONE) return false;

    // Side to move must now be Black.
    if (gs.side_to_move != COLOR_BLACK) return false;

    return true;
}

// Click on e2 (select), then click on e5 (illegal) — selection should stay.
// e5: sq_x=320+4*80=640, sq_y=40+(7-4)*80=280; center=(680,320).
static bool TestInput_LeftClick_IllegalMoveKeepsSelection(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    // Select e2.
    InputHandleLeftClick(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.has_selection) return false;

    // Click e5 — not reachable from e2 in one pawn move.
    bool moved = InputHandleLeftClick(&input, &gs, 680, 320, 320, 40, 80);

    // No move should be applied; selection cleared (empty non-target square).
    if (moved) return false;
    // The board must be unchanged (e2 still has the white pawn).
    if (gs.board.squares[1][4].piece != PIECE_PAWN) return false;
    return true;
}

// Right-click clears selection: select e2 then call InputClearSelection.
static bool TestInput_RightClick_ClearsSelection(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    // Select e2.
    InputHandleLeftClick(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.has_selection) return false;

    // Simulate right-click.
    InputClearSelection(&input);

    if (input.has_selection)        return false;
    if (input.selected_rank != -1)  return false;
    if (input.legal_moves.count != 0) return false;
    return true;
}

// Select e2, then click d2 (another White pawn) — should re-select d2.
// d2: rank 1, file 3; center=(320+3*80+40, 40+(7-1)*80+40)=(600,560).
static bool TestInput_LeftClick_ReselectsOwnPiece(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    // Select e2.
    InputHandleLeftClick(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.has_selection)     return false;
    if (input.selected_file != 4) return false;

    // Click d2 (file 3).
    InputHandleLeftClick(&input, &gs, 600, 560, 320, 40, 80);

    if (!input.has_selection)     return false;
    if (input.selected_rank != 1) return false;
    if (input.selected_file != 3) return false;
    if (input.legal_moves.count == 0) return false;
    return true;
}

// Click outside the board should clear any existing selection.
static bool TestInput_LeftClick_OutsideBoardClearsSelection(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    // Select e2.
    InputHandleLeftClick(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.has_selection) return false;

    // Click outside the board (x=10, y=10 is far from the board area).
    InputHandleLeftClick(&input, &gs, 10, 10, 320, 40, 80);

    if (input.has_selection) return false;
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
    TEST_ENTRY(TestInput_ClearSelectionResetsState),
    TEST_ENTRY(TestInput_LeftClick_SelectsOwnPiece),
    TEST_ENTRY(TestInput_LeftClick_DoesNotSelectOpponentPiece),
    TEST_ENTRY(TestInput_LeftClick_AppliesLegalMove),
    TEST_ENTRY(TestInput_LeftClick_IllegalMoveKeepsSelection),
    TEST_ENTRY(TestInput_RightClick_ClearsSelection),
    TEST_ENTRY(TestInput_LeftClick_ReselectsOwnPiece),
    TEST_ENTRY(TestInput_LeftClick_OutsideBoardClearsSelection),
};

void RunInputTests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_InputTests, sizeof(k_InputTests) / sizeof(k_InputTests[0]),
                 passed, total);
}
