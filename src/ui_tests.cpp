#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "renderer.h"
#include "moves.h"
#include "ui.h"
#include "tests.h"

static AppMemory* s_Memory;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Initialises a renderer into *rs from the test_scratch arena.
// Returns false (allowing the calling test to return false) when the arena is
// exhausted — graceful failure rather than an ASSERT crash.
static bool MakeRenderer(int32 w, int32 h, RendererState* rs)
{
    Arena* arena = &s_Memory->test_scratch;
    return RendererInit(rs, arena, w, h);
}

static bool PixelEq(Pixel a, Pixel b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// DrawBoard must not crash on a freshly initialized GameState.
static bool TestUI_DrawBoard_NoCrash(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // board_x=0, board_y=0, square_size=80, no selection, no legal moves
    DrawBoard(&rs, &gs, 0, 0, 80, -1, -1, nullptr);
    return true;
}

// The center of the selected square must be exactly BOARD_SELECTED.
// Use an empty square (e4, rank 3 / file 4) so no piece overwrites the result.
static bool TestUI_DrawBoard_SelectedSquareIsHighlighted(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Select rank 3 / file 4 (e4 — always empty at game start).
    // square_size=80, board at (0,0).
    // sq_y = (7 - 3) * 80 = 320; sq_x = 4 * 80 = 320.
    // Centre pixel: (320+40, 320+40) = (360, 360).
    DrawBoard(&rs, &gs, 0, 0, 80, 3, 4, nullptr);

    Pixel center_px = rs.pixels[(int32)360 * rs.width + (int32)360];

    // The center must be exactly BOARD_SELECTED; any other value means
    // selection highlighting is broken (or color parity regressed).
    Pixel selected = { 105, 151, 130, 0 };  // BOARD_SELECTED (BGRA)
    if (!PixelEq(center_px, selected)) return false;

    return true;
}

// A legal-move dot must appear only on a valid target square, not elsewhere.
static bool TestUI_DrawBoard_LegalMoveDotOnTargetOnly(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Build a MoveList with one target: rank 2, file 4 (e3 — one of the
    // squares a White e-pawn can advance to after the game starts).
    MoveList moves = {};
    Move m = {};
    m.from_rank = 1; m.from_file = 4;
    m.to_rank   = 2; m.to_file   = 4;
    moves.moves[moves.count++] = m;

    // Draw without a selected square but with the legal move list.
    DrawBoard(&rs, &gs, 0, 0, 80, -1, -1, &moves);

    // Target square centre: rank 2, file 4.
    // y = (7 - 2) * 80 + 40 = 440; x = 4 * 80 + 40 = 360.
    Pixel target_px = rs.pixels[(int32)440 * rs.width + (int32)360];

    // The center of an empty target square is overwritten by the move-dot circle,
    // so it must be exactly BOARD_MOVE_DOT.
    Pixel move_dot = { 64, 111, 100, 0 };  // BOARD_MOVE_DOT (BGRA)
    if (!PixelEq(target_px, move_dot)) return false;

    // A non-target square (rank 4, file 0 — a4) must keep its plain board color.
    // y = (7 - 4) * 80 + 40 = 280; x = 0 * 80 + 40 = 40.
    // rank 4 + file 0 = 4 (even) → dark square.
    Pixel dark_sq = { 99, 136, 181, 0 };
    Pixel non_target_px = rs.pixels[(int32)280 * rs.width + (int32)40];
    if (!PixelEq(non_target_px, dark_sq)) return false;

    return true;
}

// DrawGameOverOverlay must not crash for any valid GameResult.
static bool TestUI_DrawGameOverOverlay_NoCrash(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    // Use the full 1280x720 layout so coordinates don't clip badly.
    if (!MakeRenderer(1280, 720, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    DrawBoard(&rs, &gs, 320, 40, 80, -1, -1, nullptr);
    DrawGameOverOverlay(&rs, GAME_WHITE_WINS, 320, 40, 80);
    DrawGameOverOverlay(&rs, GAME_BLACK_WINS, 320, 40, 80);
    DrawGameOverOverlay(&rs, GAME_DRAW,       320, 40, 80);
    return true;
}

// A pixel inside the restart button must register as a hit.
// With board_x=320, board_y=40, square_size=80:
//   board_px = 640; overlay_w=320; overlay_h=160
//   ox = 320 + (640-320)/2 = 480; oy = 40 + (640-160)/2 = 280
//   btn_x = 480+20=500; btn_y = 280+60+20=360; btn_w=280; btn_h=60
//   Centre of button: (500+140, 360+30) = (640, 390)
static bool TestUI_IsRestartButtonHit_InsideButton(void)
{
    // Centre of the restart button.
    if (!IsRestartButtonHit(640, 390, 320, 40, 80)) return false;
    // Top-left corner (inclusive).
    if (!IsRestartButtonHit(500, 360, 320, 40, 80)) return false;
    // Bottom-right corner (last valid pixel: btn_x+btn_w-1, btn_y+btn_h-1).
    if (!IsRestartButtonHit(779, 419, 320, 40, 80)) return false;
    return true;
}

// A pixel outside the restart button must not register as a hit.
static bool TestUI_IsRestartButtonHit_OutsideButton(void)
{
    // One pixel above the button.
    if (IsRestartButtonHit(640, 359, 320, 40, 80)) return false;
    // One pixel below the button.
    if (IsRestartButtonHit(640, 420, 320, 40, 80)) return false;
    // One pixel to the left.
    if (IsRestartButtonHit(499, 390, 320, 40, 80)) return false;
    // One pixel to the right.
    if (IsRestartButtonHit(780, 390, 320, 40, 80)) return false;
    return true;
}


static const TestEntry k_UITests[] = {
    TEST_ENTRY(TestUI_DrawBoard_NoCrash),
    TEST_ENTRY(TestUI_DrawBoard_SelectedSquareIsHighlighted),
    TEST_ENTRY(TestUI_DrawBoard_LegalMoveDotOnTargetOnly),
    TEST_ENTRY(TestUI_DrawGameOverOverlay_NoCrash),
    TEST_ENTRY(TestUI_IsRestartButtonHit_InsideButton),
    TEST_ENTRY(TestUI_IsRestartButtonHit_OutsideButton),
};

void RunUITests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_UITests, sizeof(k_UITests) / sizeof(k_UITests[0]),
                 passed, total);
}
