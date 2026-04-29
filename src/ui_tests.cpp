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

// ---------------------------------------------------------------------------
// Check highlight tests
// ---------------------------------------------------------------------------

// When the side to move (White) is in check, the top-left area of the king
// square (e1 — rank 0, file 4) must be exactly BOARD_CHECK.
// Position: White King e1, Black Rook e8, Black King a8 — Black's rook
// attacks the white king along the e-file.
// Sample point (322, 562) is in the square background above the king piece.
static bool TestUI_CheckHighlight_KingSquareIsRed(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Clear all squares, then place only three pieces.
    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs.board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs.board.squares[0][4] = { PIECE_KING, COLOR_WHITE }; // White King e1
    gs.board.squares[7][4] = { PIECE_ROOK, COLOR_BLACK }; // Black Rook e8 (attacks e-file)
    gs.board.squares[7][0] = { PIECE_KING, COLOR_BLACK }; // Black King a8
    gs.side_to_move             = COLOR_WHITE;
    gs.castling_white_kingside  = false;
    gs.castling_white_queenside = false;
    gs.castling_black_kingside  = false;
    gs.castling_black_queenside = false;

    // White king at e1: sq_x = file*80 = 320, sq_y = (7-0)*80 = 560.
    // Sample (322, 562) is in the top-left corner of the square, above the king piece.
    DrawBoard(&rs, &gs, 0, 0, 80, -1, -1, nullptr);

    Pixel sample = rs.pixels[(int32)562 * rs.width + (int32)322];
    Pixel expected = { 50, 50, 200, 0 }; // BOARD_CHECK (BGRA)
    if (!PixelEq(sample, expected)) return false;

    return true;
}

// When the side to move (White) is NOT in check, the king square (e1) must
// retain its plain board colour (BOARD_DARK for rank 0, file 4 — even sum).
// Position: two kings only, no attacking pieces.
static bool TestUI_CheckHighlight_NoHighlightWhenSafe(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Clear all squares, two kings only.
    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs.board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs.board.squares[0][4] = { PIECE_KING, COLOR_WHITE }; // White King e1
    gs.board.squares[7][0] = { PIECE_KING, COLOR_BLACK }; // Black King a8
    gs.side_to_move             = COLOR_WHITE;
    gs.castling_white_kingside  = false;
    gs.castling_white_queenside = false;
    gs.castling_black_kingside  = false;
    gs.castling_black_queenside = false;

    // e1: rank 0 + file 4 = 4 (even) → BOARD_DARK.
    // Sample (322, 562): top-left corner of e1 square (above king piece).
    DrawBoard(&rs, &gs, 0, 0, 80, -1, -1, nullptr);

    Pixel sample = rs.pixels[(int32)562 * rs.width + (int32)322];
    Pixel dark   = { 99, 136, 181, 0 }; // BOARD_DARK (BGRA)
    if (!PixelEq(sample, dark)) return false;

    return true;
}

// ---------------------------------------------------------------------------
// Status overlay tests
// ---------------------------------------------------------------------------

// Helper: set up the board for White checking Black via a single rook.
// Used to verify the check highlight for the Black king.
// The White king square e1 (rank 0, file 4) is NOT the king in check.
static bool TestUI_CheckHighlight_BlackKingIsRed(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Clear all squares.
    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs.board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // Black King e8 (rank 7, file 4), White Rook e1 (rank 0, file 4), White King a1
    gs.board.squares[7][4] = { PIECE_KING, COLOR_BLACK }; // Black King e8
    gs.board.squares[0][4] = { PIECE_ROOK, COLOR_WHITE }; // White Rook e1 attacks e-file
    gs.board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White King a1
    gs.side_to_move             = COLOR_BLACK;
    gs.castling_white_kingside  = false;
    gs.castling_white_queenside = false;
    gs.castling_black_kingside  = false;
    gs.castling_black_queenside = false;

    // Black King e8: sq_x = 4*80 = 320, sq_y = (7-7)*80 = 0.
    // Sample (322, 2): top-left corner of e8 square (above any piece drawing).
    DrawBoard(&rs, &gs, 0, 0, 80, -1, -1, nullptr);

    Pixel sample   = rs.pixels[(int32)2 * rs.width + (int32)322];
    Pixel expected = { 50, 50, 200, 0 }; // BOARD_CHECK (BGRA)
    if (!PixelEq(sample, expected)) return false;

    return true;
}

// After Fool's Mate (GAME_BLACK_WINS), DrawStatusOverlay must paint the banner
// background over the board centre.  The banner spans board_x to board_x+640
// and is vertically centred on the board.
// With board_x=0, board_y=0, square_size=80:
//   board_px = 640, banner_h = 33, banner_y = (640-33)/2 = 303.
//   Sample (50, 315) is inside the banner, left of the text (text_x >= 123).
static bool TestUI_CheckmateOverlay_BannerVisible(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Apply Fool's Mate: 1. f3 e5  2. g4 Qh4#
    Move m1 = {}; m1.from_rank = 1; m1.from_file = 5; m1.to_rank = 2; m1.to_file = 5;
    ApplyMove(&gs, &m1);
    Move m2 = {}; m2.from_rank = 6; m2.from_file = 4; m2.to_rank = 4; m2.to_file = 4;
    ApplyMove(&gs, &m2);
    Move m3 = {}; m3.from_rank = 1; m3.from_file = 6; m3.to_rank = 3; m3.to_file = 6;
    ApplyMove(&gs, &m3);
    Move m4 = {}; m4.from_rank = 7; m4.from_file = 3; m4.to_rank = 3; m4.to_file = 7;
    ApplyMove(&gs, &m4);
    // White is now in checkmate: GAME_BLACK_WINS.

    DrawStatusOverlay(&rs, &gs, 0, 0, 80);

    // Sample (50, 315): inside the banner background, left of any text.
    Pixel sample   = rs.pixels[(int32)315 * rs.width + (int32)50];
    Pixel expected = { 20, 20, 20, 0 }; // STATUS_BANNER_BG (BGRA)
    if (!PixelEq(sample, expected)) return false;

    return true;
}

// After a stalemate position (GAME_DRAW), DrawStatusOverlay must paint the banner.
// White: King c6, Queen b6.  Black: King a8.  Side to move: BLACK.
// Black has no legal moves and is not in check → GAME_DRAW.
static bool TestUI_StalemateOverlay_BannerVisible(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs.board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs.board.squares[7][0] = { PIECE_KING,  COLOR_BLACK }; // Black King a8
    gs.board.squares[5][2] = { PIECE_KING,  COLOR_WHITE }; // White King c6
    gs.board.squares[5][1] = { PIECE_QUEEN, COLOR_WHITE }; // White Queen b6
    gs.side_to_move             = COLOR_BLACK;
    gs.castling_white_kingside  = false;
    gs.castling_white_queenside = false;
    gs.castling_black_kingside  = false;
    gs.castling_black_queenside = false;

    DrawStatusOverlay(&rs, &gs, 0, 0, 80);

    // Banner at y=303, sample (50, 315).
    Pixel sample   = rs.pixels[(int32)315 * rs.width + (int32)50];
    Pixel expected = { 20, 20, 20, 0 }; // STATUS_BANNER_BG (BGRA)
    if (!PixelEq(sample, expected)) return false;

    return true;
}

// For an ongoing game, DrawStatusOverlay must be a no-op: the sample point
// at the would-be banner location retains the clear colour.
static bool TestUI_OngoingGame_NoOverlay(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs); // standard starting position — GAME_ONGOING

    // Fill with a known background so we can verify DrawStatusOverlay is silent.
    Pixel bg = { 40, 40, 40, 0 };
    ClearBuffer(&rs, bg);

    DrawStatusOverlay(&rs, &gs, 0, 0, 80);

    // If no overlay was drawn, pixel at (50, 315) (in the would-be banner) stays bg.
    Pixel sample = rs.pixels[(int32)315 * rs.width + (int32)50];
    if (!PixelEq(sample, bg)) return false;

    return true;
}

// After a White-wins checkmate (GAME_WHITE_WINS), DrawStatusOverlay must:
//   1. Paint the dark banner over the board centre.
//   2. Render the correct "CHECKMATE - WHITE WINS" glyph, not "BLACK WINS".
// Position: White King f6, White Queen g7, Black King h8 — side to move BLACK.
//   Black King h8 is in check from the Queen (diagonal) and has no legal escapes
//   (g8/h7 attacked by queen, g7 defended by White King) → GAME_WHITE_WINS.
//
// Banner geometry with board_x=0, board_y=0, square_size=80:
//   board_px=640, banner_h=33, banner_y=303, text_y=309.
//   "CHECKMATE - WHITE WINS" (22 chars): text_w=393, text_x=123.
//   First glyph 'C' row 1 (mask=16=0b10000): col 0 set at scale 3
//   → text pixel at (123, 312) must be STATUS_TEXT_COLOR.
static bool TestUI_CheckmateOverlay_WhiteWins_BannerAndTextPixel(void)
{
    ArenaReset(&s_Memory->test_scratch);
    RendererState rs = {};
    if (!MakeRenderer(640, 640, &rs)) return false;

    GameState gs = {};
    InitGameState(&gs);

    // Clear all squares.
    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs.board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White King f6 (rank 5, file 5), White Queen g7 (rank 6, file 6),
    // Black King h8 (rank 7, file 7).
    gs.board.squares[5][5] = { PIECE_KING,  COLOR_WHITE }; // White King f6
    gs.board.squares[6][6] = { PIECE_QUEEN, COLOR_WHITE }; // White Queen g7
    gs.board.squares[7][7] = { PIECE_KING,  COLOR_BLACK }; // Black King h8
    gs.side_to_move             = COLOR_BLACK;
    gs.castling_white_kingside  = false;
    gs.castling_white_queenside = false;
    gs.castling_black_kingside  = false;
    gs.castling_black_queenside = false;

    DrawStatusOverlay(&rs, &gs, 0, 0, 80);

    // 1. Banner background at (50, 315) — left of text, inside banner.
    Pixel banner_px = rs.pixels[(int32)315 * rs.width + (int32)50];
    Pixel banner_bg = { 20, 20, 20, 0 }; // STATUS_BANNER_BG (BGRA)
    if (!PixelEq(banner_px, banner_bg)) return false;

    // 2. Text pixel from first glyph 'C':
    //    row 1 of k_C = 16 = 0b10000 → col 0 set.
    //    x = text_x + 0*scale = 123, y = text_y + 1*scale = 309 + 3 = 312.
    Pixel text_px    = rs.pixels[(int32)312 * rs.width + (int32)123];
    Pixel text_color = { 220, 220, 220, 0 }; // STATUS_TEXT_COLOR (BGRA)
    if (!PixelEq(text_px, text_color)) return false;

    return true;
}

static const TestEntry k_UITests[] = {
    TEST_ENTRY(TestUI_DrawBoard_NoCrash),
    TEST_ENTRY(TestUI_DrawBoard_SelectedSquareIsHighlighted),
    TEST_ENTRY(TestUI_DrawBoard_LegalMoveDotOnTargetOnly),
    TEST_ENTRY(TestUI_CheckHighlight_KingSquareIsRed),
    TEST_ENTRY(TestUI_CheckHighlight_NoHighlightWhenSafe),
    TEST_ENTRY(TestUI_CheckHighlight_BlackKingIsRed),
    TEST_ENTRY(TestUI_CheckmateOverlay_BannerVisible),
    TEST_ENTRY(TestUI_StalemateOverlay_BannerVisible),
    TEST_ENTRY(TestUI_OngoingGame_NoOverlay),
    TEST_ENTRY(TestUI_CheckmateOverlay_WhiteWins_BannerAndTextPixel),
};
void RunUITests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_UITests, sizeof(k_UITests) / sizeof(k_UITests[0]),
                 passed, total);
}
