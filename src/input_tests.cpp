#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "moves.h"
#include "input.h"
#include "ui.h"
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
// Promotion picker tests
// ---------------------------------------------------------------------------

// Helper: set up a GameState with a White pawn at e7 (rank 6, file 4), both
// kings present, and a clear e8 so the pawn can promote.  White to move.
// e8 (rank 7, file 4) is the Black king's starting square, so we relocate
// the Black king to h5 (rank 4, file 7 — empty in the starting position)
// before clearing e8, keeping the board legal.
static void SetupPromoPosition(GameState* gs)
{
    InitGameState(gs);
    // Move White pawn from e2 to e7 (one step from promotion on e8).
    gs->board.squares[1][4] = { PIECE_NONE, COLOR_NONE }; // clear e2 White pawn
    gs->board.squares[6][4] = { PIECE_PAWN, COLOR_WHITE }; // White pawn at e7 (overwrites Black pawn)
    // Relocate the Black king from e8 to h5 so e8 is open for promotion.
    gs->board.squares[7][4] = { PIECE_NONE, COLOR_NONE }; // clear e8 (was Black king)
    gs->board.squares[4][7] = { PIECE_KING, COLOR_BLACK }; // Black king to h5
    // Disable Black castling — king is no longer on its start square.
    gs->castling_black_kingside  = false;
    gs->castling_black_queenside = false;
}

// Dragging a White pawn from e7 and dropping it on e8 must enter pending
// promotion rather than immediately applying a move.
// e7 center: sq_x=640, sq_y=120; center=(680,160)
// e8 center: sq_x=640, sq_y=40;  center=(680,80)
static bool TestInput_DragEnd_SetsPromotionPending(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80); // pick up e7
    if (!input.dragging) return false;

    bool moved = InputHandleDragEnd(&input, &gs, 680, 80, 320, 40, 80); // drop on e8

    if (moved)                        return false; // no move yet
    if (input.dragging)               return false; // drag ended
    if (!input.pending_promotion)     return false;
    if (input.promo_from_rank != 6)   return false;
    if (input.promo_from_file != 4)   return false;
    if (input.promo_to_rank   != 7)   return false;
    if (input.promo_to_file   != 4)   return false;
    // Board unchanged — pawn still at e7
    if (gs.board.squares[6][4].piece != PIECE_PAWN) return false;
    if (gs.board.squares[7][4].piece != PIECE_NONE) return false;
    return true;
}

// Clicking rank 7, file 4 (index 0 = PIECE_QUEEN) during pending promotion
// must apply a queen promotion.
static bool TestInput_PromotionClick_AppliesQueen(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 80, 320, 40, 80);
    if (!input.pending_promotion) return false;

    // Click e8 (rank 7, file 4) — index 0 = Queen
    bool moved = InputHandlePromotionClick(&input, &gs, 680, 80, 320, 40, 80);

    if (!moved)                                       return false;
    if (input.pending_promotion)                      return false; // cleared
    if (gs.board.squares[7][4].piece != PIECE_QUEEN)  return false;
    if (gs.board.squares[7][4].color != COLOR_WHITE)  return false;
    if (gs.board.squares[6][4].piece != PIECE_NONE)   return false; // e7 empty
    if (gs.side_to_move != COLOR_BLACK)               return false;
    return true;
}

// Clicking rank 4, file 4 (index 3 = PIECE_KNIGHT) must apply a knight
// underpromotion.
// rank 4 center: sq_y = 40 + (7-4)*80 = 280 + 40 = 320; center=(680,320)
static bool TestInput_PromotionClick_AppliesKnight(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 80, 320, 40, 80);
    if (!input.pending_promotion) return false;

    // Click rank 4, file 4 — index 3 = Knight
    bool moved = InputHandlePromotionClick(&input, &gs, 680, 320, 320, 40, 80);

    if (!moved)                                        return false;
    if (input.pending_promotion)                       return false;
    if (gs.board.squares[7][4].piece != PIECE_KNIGHT)  return false;
    if (gs.board.squares[7][4].color != COLOR_WHITE)   return false;
    return true;
}

// Clicking outside the picker column (wrong file) must cancel the pending
// promotion without applying any move.
// d4 (rank 3, file 3) is outside the picker (file 4).
// d4 center: sq_x=320+3*80=560, sq_y=40+(7-3)*80=360; center=(600,400)
static bool TestInput_PromotionClick_CancelsOnWrongFile(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 80, 320, 40, 80);
    if (!input.pending_promotion) return false;

    bool moved = InputHandlePromotionClick(&input, &gs, 600, 400, 320, 40, 80);

    if (moved)                        return false;
    if (input.pending_promotion)      return false; // cancelled
    if (gs.board.squares[6][4].piece != PIECE_PAWN) return false; // board unchanged
    return true;
}

// Clicking below the picker range (rank 3 for White, same file) must cancel.
// rank 3, file 4 center: sq_y=40+(7-3)*80=360; center=(680,400)
static bool TestInput_PromotionClick_CancelsOnOutsideRange(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 80, 320, 40, 80);
    if (!input.pending_promotion) return false;

    // Rank 3 at file 4 — for White, idx = 7-3 = 4 which is out of [0,4)
    bool moved = InputHandlePromotionClick(&input, &gs, 680, 400, 320, 40, 80);

    if (moved)               return false;
    if (input.pending_promotion) return false; // cancelled
    return true;
}

// Clicking outside the board during a pending promotion must cancel.
static bool TestInput_PromotionClick_CancelsOutsideBoard(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 80, 320, 40, 80);
    if (!input.pending_promotion) return false;

    bool moved = InputHandlePromotionClick(&input, &gs, 10, 10, 320, 40, 80);

    if (moved)               return false;
    if (input.pending_promotion) return false;
    return true;
}

// InputHandlePromotionClick when not in pending_promotion state must be a
// safe no-op returning false.
static bool TestInput_PromotionClick_NoOpWhenNotPending(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    bool moved = InputHandlePromotionClick(&input, &gs, 680, 80, 320, 40, 80);

    if (moved)               return false;
    if (input.pending_promotion) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black promotion picker tests
// ---------------------------------------------------------------------------

// Helper: set up a position with only a Black pawn at e2 (rank 1, file 4)
// ready to promote to e1 (rank 0, file 4), with both kings on the board and
// the promotion square clear.  Black to move.
static void SetupBlackPromoPosition(GameState* gs)
{
    InitGameState(gs);
    gs->side_to_move = COLOR_BLACK;
    // Remove the Black pawn from its standard e7 start square.
    gs->board.squares[6][4] = { PIECE_NONE, COLOR_NONE };
    // Place a Black pawn at e2 (one step from promotion), overwriting the
    // White pawn that occupies that square in the standard opening position.
    gs->board.squares[1][4] = { PIECE_PAWN, COLOR_BLACK };
    // Clear e1 (White king's standard square) to open the promotion square.
    gs->board.squares[0][4] = { PIECE_NONE, COLOR_NONE };
    // Place the White king on a3 (rank 2, file 0), which is empty in the
    // standard opening and safely away from e1 and the Black king.
    gs->board.squares[2][0] = { PIECE_KING, COLOR_WHITE };
    // Disable castling because the White king is no longer on its start square.
    gs->castling_white_kingside  = false;
    gs->castling_white_queenside = false;
}

// Dragging a Black pawn from e2 and dropping it on e1 must enter pending
// promotion rather than immediately applying a move.
// e2 (rank 1, file 4): sq_y=40+(7-1)*80=520; center=(680,560)
// e1 (rank 0, file 4): sq_y=40+(7-0)*80=600; center=(680,640)
static bool TestInput_DragEnd_SetsPromotionPending_Black(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupBlackPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80); // pick up e2
    if (!input.dragging) return false;

    bool moved = InputHandleDragEnd(&input, &gs, 680, 640, 320, 40, 80); // drop on e1

    if (moved)                        return false; // no move yet
    if (input.dragging)               return false; // drag ended
    if (!input.pending_promotion)     return false;
    if (input.promo_from_rank != 1)   return false;
    if (input.promo_from_file != 4)   return false;
    if (input.promo_to_rank   != 0)   return false;
    if (input.promo_to_file   != 4)   return false;
    // Board unchanged — Black pawn still at e2.
    if (gs.board.squares[1][4].piece != PIECE_PAWN) return false;
    if (gs.board.squares[0][4].piece != PIECE_NONE) return false;
    return true;
}

// Clicking rank 0, file 4 (e1, index 0 = PIECE_QUEEN) during a Black pending
// promotion must apply a queen promotion.
// rank 0, file 4 center: sq_y=40+(7-0)*80=600; center=(680,640)
static bool TestInput_PromotionClick_AppliesQueen_Black(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupBlackPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 640, 320, 40, 80);
    if (!input.pending_promotion) return false;

    // Click e1 (rank 0, file 4) — Black picker index 0 = Queen.
    bool moved = InputHandlePromotionClick(&input, &gs, 680, 640, 320, 40, 80);

    if (!moved)                                       return false;
    if (input.pending_promotion)                      return false; // cleared
    if (gs.board.squares[0][4].piece != PIECE_QUEEN)  return false;
    if (gs.board.squares[0][4].color != COLOR_BLACK)  return false;
    if (gs.board.squares[1][4].piece != PIECE_NONE)   return false; // e2 empty
    if (gs.side_to_move != COLOR_WHITE)               return false;
    return true;
}

// Clicking rank 3, file 4 (index 3 = PIECE_KNIGHT) must apply a knight
// underpromotion for Black.
// rank 3, file 4 center: sq_y=40+(7-3)*80=360; center=(680,400)
static bool TestInput_PromotionClick_AppliesKnight_Black(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupBlackPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 640, 320, 40, 80);
    if (!input.pending_promotion) return false;

    // Click rank 3, file 4 — Black picker index 3 = Knight.
    bool moved = InputHandlePromotionClick(&input, &gs, 680, 400, 320, 40, 80);

    if (!moved)                                        return false;
    if (input.pending_promotion)                       return false;
    if (gs.board.squares[0][4].piece != PIECE_KNIGHT)  return false;
    if (gs.board.squares[0][4].color != COLOR_BLACK)   return false;
    return true;
}

// Clicking rank 4, file 4 (one step below the picker) must cancel the Black
// pending promotion without applying any move.
// rank 4, file 4 center: sq_y=40+(7-4)*80=280; center=(680,320)
static bool TestInput_PromotionClick_CancelsOnOutsideRange_Black(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupBlackPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80);
    InputHandleDragEnd(&input, &gs, 680, 640, 320, 40, 80);
    if (!input.pending_promotion) return false;

    // For Black, idx = rank = 4, which is outside [0, 4) — must cancel.
    bool moved = InputHandlePromotionClick(&input, &gs, 680, 320, 320, 40, 80);

    if (moved)                   return false;
    if (input.pending_promotion) return false; // cancelled
    return true;
}

// ---------------------------------------------------------------------------
// InputRestart tests
// ---------------------------------------------------------------------------

// Restart from idle — verify all GameState fields are reset to starting values.
static bool TestInput_Restart_ResetsGameState(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    // Dirty the game state by applying a move (1. e4), which changes
    // side_to_move and sets an en-passant target.
    Move m = {};
    m.from_rank = 1; m.from_file = 4;
    m.to_rank   = 3; m.to_file   = 4;
    ApplyMove(&gs, &m);

    InputState input = {};
    InputInit(&input);

    InputRestart(&input, &gs);

    // side_to_move must be WHITE.
    if (gs.side_to_move != COLOR_WHITE)       return false;

    // En-passant must be cleared.
    if (gs.en_passant_rank != -1)             return false;
    if (gs.en_passant_file != -1)             return false;

    // All castling rights must be restored.
    if (!gs.castling_white_kingside)          return false;
    if (!gs.castling_white_queenside)         return false;
    if (!gs.castling_black_kingside)          return false;
    if (!gs.castling_black_queenside)         return false;

    // White pawns must be back on rank 1.
    for (int32 f = 0; f < 8; ++f)
    {
        if (gs.board.squares[1][f].piece != PIECE_PAWN)    return false;
        if (gs.board.squares[1][f].color != COLOR_WHITE)   return false;
    }

    // Black pawns must be on rank 6.
    for (int32 f = 0; f < 8; ++f)
    {
        if (gs.board.squares[6][f].piece != PIECE_PAWN)    return false;
        if (gs.board.squares[6][f].color != COLOR_BLACK)   return false;
    }

    // Rank 3 must be empty (the moved pawn was returned to e2 by InitGameState).
    for (int32 f = 0; f < 8; ++f)
        if (gs.board.squares[3][f].piece != PIECE_NONE)    return false;

    return true;
}

// Restart must clear all input sub-state.
static bool TestInput_Restart_ClearsInputState(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    InputRestart(&input, &gs);

    if (input.dragging)          return false;
    if (input.pending_promotion) return false;
    if (input.drag_from_rank != -1) return false;
    if (input.drag_from_file != -1) return false;
    if (input.drag_cursor_x  != 0)  return false;
    if (input.drag_cursor_y  != 0)  return false;
    if (input.legal_moves.count != 0) return false;
    if (input.promo_from_rank != -1) return false;
    if (input.promo_from_file != -1) return false;
    if (input.promo_to_rank   != -1) return false;
    if (input.promo_to_file   != -1) return false;

    return true;
}

// Restart while an active drag is in progress must not corrupt state.
static bool TestInput_Restart_SafeDuringActiveDrag(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    InputState input = {};
    InputInit(&input);

    // Start a drag of the White e-pawn.
    // e2 center: sq_x=320+4*80=640, sq_y=40+(7-1)*80=520; center=(680,560)
    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.dragging) return false;

    InputRestart(&input, &gs);

    // After restart, drag must be cleared and game in starting state.
    if (input.dragging)               return false;
    if (input.pending_promotion)      return false;
    if (gs.side_to_move != COLOR_WHITE) return false;

    // The next drag start must work as if starting fresh.
    InputHandleDragStart(&input, &gs, 680, 560, 320, 40, 80);
    if (!input.dragging) return false;

    return true;
}

// Restart while a pending promotion is active must not corrupt state.
static bool TestInput_Restart_SafeDuringPendingPromotion(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    SetupPromoPosition(&gs);

    InputState input = {};
    InputInit(&input);

    // Drag e7 pawn to e8 to enter pending promotion.
    InputHandleDragStart(&input, &gs, 680, 160, 320, 40, 80);
    InputHandleDragEnd  (&input, &gs, 680,  80, 320, 40, 80);
    if (!input.pending_promotion) return false;

    InputRestart(&input, &gs);

    if (input.pending_promotion)        return false;
    if (input.dragging)                 return false;
    if (gs.side_to_move != COLOR_WHITE) return false;
    // Board must be in the standard starting position.
    if (gs.board.squares[6][4].piece != PIECE_PAWN) return false;  // e7 pawn restored
    if (gs.board.squares[7][4].piece != PIECE_NONE) return false;  // e8 empty again

    return true;
}

// ---------------------------------------------------------------------------
// Game-over click routing contract
// ---------------------------------------------------------------------------
// These tests exercise the same conditional logic performed in WM_LBUTTONDOWN:
//   if (IsRestartButtonHit(px, py, ...)) InputRestart(input, gs);
// They verify that:
//   (a) a click inside the restart button triggers a full state reset, and
//   (b) a click outside the button leaves all state unchanged.

// A click at the restart button position must produce a clean game state.
// Simulates: WM_LBUTTONDOWN game-over routing where IsRestartButtonHit → true.
// Board layout: board_x=320, board_y=40, square_size=80.
// Restart button centre: (640, 390) — confirmed by IsRestartButtonHit tests.
static bool TestInput_GameOverClick_RestartButtonResetsState(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    // Dirty the game state by advancing side_to_move and clearing en-passant.
    Move m = {};
    m.from_rank = 1; m.from_file = 4;
    m.to_rank   = 3; m.to_file   = 4;
    ApplyMove(&gs, &m);

    InputState input = {};
    InputInit(&input);

    // Simulate dragging so there is also dirty input state.
    InputHandleDragStart(&input, &gs, 680, 480, 320, 40, 80);

    // Click is on the restart button — IsRestartButtonHit returns true,
    // so the WM_LBUTTONDOWN handler calls InputRestart.
    int32 px = 640, py = 390;
    if (IsRestartButtonHit(px, py, 320, 40, 80))
        InputRestart(&input, &gs);

    // Game state must be reset.
    if (gs.side_to_move != COLOR_WHITE)       return false;
    if (gs.en_passant_rank != -1)             return false;
    if (gs.en_passant_file != -1)             return false;
    if (!gs.castling_white_kingside)          return false;
    if (!gs.castling_white_queenside)         return false;
    if (!gs.castling_black_kingside)          return false;
    if (!gs.castling_black_queenside)         return false;
    // Pawns must be back on their starting ranks.
    for (int32 f = 0; f < 8; ++f)
    {
        if (gs.board.squares[1][f].piece != PIECE_PAWN)  return false;
        if (gs.board.squares[1][f].color != COLOR_WHITE) return false;
        if (gs.board.squares[6][f].piece != PIECE_PAWN)  return false;
        if (gs.board.squares[6][f].color != COLOR_BLACK) return false;
    }

    // Input state must also be cleared.
    if (input.dragging)          return false;
    if (input.pending_promotion) return false;

    return true;
}

// A click outside the restart button must leave game state unchanged.
// Simulates: WM_LBUTTONDOWN game-over routing where IsRestartButtonHit → false.
// Use a point one pixel above the button (640, 359) to verify the guard.
static bool TestInput_GameOverClick_NonButtonClickDoesNothing(void)
{
    ArenaReset(&s_Memory->test_scratch);

    GameState gs = {};
    InitGameState(&gs);

    // Apply a move so the position is no longer the starting one.
    Move m = {};
    m.from_rank = 1; m.from_file = 4;
    m.to_rank   = 3; m.to_file   = 4;
    ApplyMove(&gs, &m);
    // Now side_to_move == COLOR_BLACK.

    InputState input = {};
    InputInit(&input);

    // Click is NOT on the restart button — handler does nothing.
    int32 px = 640, py = 359; // one pixel above the button
    if (IsRestartButtonHit(px, py, 320, 40, 80))
        InputRestart(&input, &gs); // must NOT be reached

    // Game state must be unchanged (still at the post-move position).
    if (gs.side_to_move != COLOR_BLACK)       return false;
    // en_passant set by the double-pawn push.
    if (gs.en_passant_rank != 2)              return false;
    if (gs.en_passant_file != 4)              return false;
    // The moved pawn must still be on rank 3.
    if (gs.board.squares[3][4].piece != PIECE_PAWN)  return false;
    if (gs.board.squares[3][4].color != COLOR_WHITE) return false;
    // Starting square must be empty.
    if (gs.board.squares[1][4].piece != PIECE_NONE)  return false;

    return true;
}


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
    TEST_ENTRY(TestInput_DragEnd_SetsPromotionPending),
    TEST_ENTRY(TestInput_PromotionClick_AppliesQueen),
    TEST_ENTRY(TestInput_PromotionClick_AppliesKnight),
    TEST_ENTRY(TestInput_PromotionClick_CancelsOnWrongFile),
    TEST_ENTRY(TestInput_PromotionClick_CancelsOnOutsideRange),
    TEST_ENTRY(TestInput_PromotionClick_CancelsOutsideBoard),
    TEST_ENTRY(TestInput_PromotionClick_NoOpWhenNotPending),
    TEST_ENTRY(TestInput_DragEnd_SetsPromotionPending_Black),
    TEST_ENTRY(TestInput_PromotionClick_AppliesQueen_Black),
    TEST_ENTRY(TestInput_PromotionClick_AppliesKnight_Black),
    TEST_ENTRY(TestInput_PromotionClick_CancelsOnOutsideRange_Black),
    TEST_ENTRY(TestInput_Restart_ResetsGameState),
    TEST_ENTRY(TestInput_Restart_ClearsInputState),
    TEST_ENTRY(TestInput_Restart_SafeDuringActiveDrag),
    TEST_ENTRY(TestInput_Restart_SafeDuringPendingPromotion),
    TEST_ENTRY(TestInput_GameOverClick_RestartButtonResetsState),
    TEST_ENTRY(TestInput_GameOverClick_NonButtonClickDoesNothing),
};

void RunInputTests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_InputTests, sizeof(k_InputTests) / sizeof(k_InputTests[0]),
                 passed, total);
}
