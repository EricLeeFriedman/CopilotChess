#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "moves.h"
#include "tests.h"

static AppMemory* s_Memory;

// ---------------------------------------------------------------------------
// Helper: find a move in list matching the given from/to coordinates.
// ---------------------------------------------------------------------------
static bool FindMove(const MoveList* list, int8 fr, int8 ff, int8 tr, int8 tf)
{
    for (int32 i = 0; i < list->count; ++i)
    {
        const Move& m = list->moves[i];
        if (m.from_rank == fr && m.from_file == ff &&
            m.to_rank   == tr && m.to_file   == tf)
        {
            return true;
        }
    }
    return false;
}

// Same as FindMove but also requires is_en_passant flag.
static bool FindEnPassantMove(const MoveList* list, int8 fr, int8 ff, int8 tr, int8 tf)
{
    for (int32 i = 0; i < list->count; ++i)
    {
        const Move& m = list->moves[i];
        if (m.from_rank == fr && m.from_file == ff &&
            m.to_rank   == tr && m.to_file   == tf &&
            m.is_en_passant)
        {
            return true;
        }
    }
    return false;
}

// Same as FindMove but also requires a promotion piece.
static bool FindPromotion(const MoveList* list, int8 fr, int8 ff, int8 tr, int8 tf, PieceType promo)
{
    for (int32 i = 0; i < list->count; ++i)
    {
        const Move& m = list->moves[i];
        if (m.from_rank == fr && m.from_file == ff &&
            m.to_rank   == tr && m.to_file   == tf &&
            m.promotion == promo)
        {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// White pawn starting double push (e-file, rank 1 -> rank 2 and rank 3).
// ---------------------------------------------------------------------------
static bool TestPawn_WhiteDoublePush(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // e-pawn (file 4, rank 1) should have both e3 and e4.
    if (!FindMove(&list, 1, 4, 2, 4)) return false; // e3
    if (!FindMove(&list, 1, 4, 3, 4)) return false; // e4
    return true;
}

// ---------------------------------------------------------------------------
// Black pawn starting double push (d-file, rank 6 -> rank 5 and rank 4).
// ---------------------------------------------------------------------------
static bool TestPawn_BlackDoublePush(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);
    gs->side_to_move = COLOR_BLACK;

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // d-pawn (file 3, rank 6) should have both d6 and d5 in chess notation,
    // i.e., to rank 5 and rank 4 in 0-indexed terms.
    if (!FindMove(&list, 6, 3, 5, 3)) return false; // d6
    if (!FindMove(&list, 6, 3, 4, 3)) return false; // d5
    return true;
}

// ---------------------------------------------------------------------------
// Blocked single push: piece on the square directly in front.
// ---------------------------------------------------------------------------
static bool TestPawn_BlockedSinglePush(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // Place a black piece on e3 (rank 2, file 4) to block the white e-pawn.
    gs->board.squares[2][4] = { PIECE_ROOK, COLOR_BLACK };

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // Neither e3 nor e4 should be reachable.
    if (FindMove(&list, 1, 4, 2, 4)) return false;
    if (FindMove(&list, 1, 4, 3, 4)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Blocked double push: square two ahead is blocked but square one is clear.
// ---------------------------------------------------------------------------
static bool TestPawn_BlockedDoublePush(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // Place a piece on e4 (rank 3, file 4); e3 is still clear.
    gs->board.squares[3][4] = { PIECE_ROOK, COLOR_BLACK };

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // e3 is reachable but e4 is not.
    if (!FindMove(&list, 1, 4, 2, 4)) return false; // e3 ok
    if ( FindMove(&list, 1, 4, 3, 4)) return false; // e4 blocked
    return true;
}

// ---------------------------------------------------------------------------
// Diagonal capture: enemy piece on the diagonal.
// ---------------------------------------------------------------------------
static bool TestPawn_DiagonalCapture(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // Clear the standard starting position and place pieces manually.
    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White pawn on d4 (rank 3, file 3).
    gs->board.squares[3][3] = { PIECE_PAWN, COLOR_WHITE };
    // Black piece on e5 (rank 4, file 4).
    gs->board.squares[4][4] = { PIECE_ROOK, COLOR_BLACK };
    // No piece on c5 (rank 4, file 2).

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // Capture on e5 should be listed.
    if (!FindMove(&list, 3, 3, 4, 4)) return false;
    // No capture on c5 (empty).
    if ( FindMove(&list, 3, 3, 4, 2)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// No capture of friendly piece on diagonal.
// ---------------------------------------------------------------------------
static bool TestPawn_NoFriendlyCapture(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White pawn on d4 (rank 3, file 3), white rook on e5 (rank 4, file 4).
    gs->board.squares[3][3] = { PIECE_PAWN,  COLOR_WHITE };
    gs->board.squares[4][4] = { PIECE_ROOK,  COLOR_WHITE };

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // Must not capture own piece.
    if (FindMove(&list, 3, 3, 4, 4)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// En passant capture: after a black double push, white captures en passant.
// ---------------------------------------------------------------------------
static bool TestPawn_EnPassant(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White pawn on e5 (rank 4, file 4).
    gs->board.squares[4][4] = { PIECE_PAWN, COLOR_WHITE };
    // Black pawn on d5 (rank 4, file 3) — just double-pushed from d7.
    gs->board.squares[4][3] = { PIECE_PAWN, COLOR_BLACK };

    // Set en passant target to d6 (rank 5, file 3).
    gs->en_passant_rank = 5;
    gs->en_passant_file = 3;

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // White should be able to capture en passant to d6.
    if (!FindEnPassantMove(&list, 4, 4, 5, 3)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Apply en passant: captured pawn removed from board.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyEnPassant(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White pawn on e5 (rank 4, file 4), black pawn on d5 (rank 4, file 3).
    gs->board.squares[4][4] = { PIECE_PAWN, COLOR_WHITE };
    gs->board.squares[4][3] = { PIECE_PAWN, COLOR_BLACK };
    gs->en_passant_rank = 5;
    gs->en_passant_file = 3;

    Move ep    = {};
    ep.from_rank     = 4; ep.from_file     = 4;
    ep.to_rank       = 5; ep.to_file       = 3;
    ep.is_en_passant = true;
    ep.promotion     = PIECE_NONE;

    ApplyMove(gs, &ep);

    // White pawn should now be on d6 (rank 5, file 3).
    if (gs->board.squares[5][3].piece != PIECE_PAWN)  return false;
    if (gs->board.squares[5][3].color != COLOR_WHITE) return false;
    // Black pawn on d5 must be gone.
    if (gs->board.squares[4][3].piece != PIECE_NONE)  return false;
    // White pawn's old square must be clear.
    if (gs->board.squares[4][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// Promotion: pawn on the 7th rank (rank 6) generates one move per piece type.
// ---------------------------------------------------------------------------
static bool TestPawn_Promotion(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White pawn on e7 (rank 6, file 4) — one push away from promotion.
    gs->board.squares[6][4] = { PIECE_PAWN, COLOR_WHITE };

    MoveList list = {};
    GeneratePawnMoves(gs, &list);

    // One move per legal promotion piece must be present.
    if (!FindPromotion(&list, 6, 4, 7, 4, PIECE_QUEEN))  return false;
    if (!FindPromotion(&list, 6, 4, 7, 4, PIECE_ROOK))   return false;
    if (!FindPromotion(&list, 6, 4, 7, 4, PIECE_BISHOP)) return false;
    if (!FindPromotion(&list, 6, 4, 7, 4, PIECE_KNIGHT)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Apply promotion: pawn replaced by queen on back rank.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White pawn on e7 (rank 6, file 4).
    gs->board.squares[6][4] = { PIECE_PAWN, COLOR_WHITE };

    Move pm    = {};
    pm.from_rank  = 6; pm.from_file  = 4;
    pm.to_rank    = 7; pm.to_file    = 4;
    pm.promotion  = PIECE_QUEEN;
    pm.is_en_passant = false;

    ApplyMove(gs, &pm);

    // e8 should now contain a white queen.
    if (gs->board.squares[7][4].piece != PIECE_QUEEN) return false;
    if (gs->board.squares[7][4].color != COLOR_WHITE) return false;
    // e7 must be clear.
    if (gs->board.squares[6][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// En passant target set after double push, cleared after next move.
// ---------------------------------------------------------------------------
static bool TestPawn_EnPassantTargetTracking(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // White e-pawn double pushes from e2 (rank 1, file 4) to e4 (rank 3, file 4).
    Move dp    = {};
    dp.from_rank = 1; dp.from_file = 4;
    dp.to_rank   = 3; dp.to_file   = 4;
    dp.promotion = PIECE_NONE;
    dp.is_en_passant = false;

    ApplyMove(gs, &dp);

    // En passant target must be e3 (rank 2, file 4).
    if (gs->en_passant_rank != 2) return false;
    if (gs->en_passant_file != 4) return false;

    // Make any other move (black a-pawn single push) to consume the en passant window.
    Move sp    = {};
    sp.from_rank = 6; sp.from_file = 0;
    sp.to_rank   = 5; sp.to_file   = 0;
    sp.promotion = PIECE_NONE;
    sp.is_en_passant = false;

    ApplyMove(gs, &sp);

    // En passant target must now be cleared.
    if (gs->en_passant_rank != -1) return false;
    if (gs->en_passant_file != -1) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Knight in the center (e4 = rank 3, file 4) — expects all 8 moves.
// ---------------------------------------------------------------------------
static bool TestKnight_CenterMoves(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White knight on e4 (rank 3, file 4).
    gs->board.squares[3][4] = { PIECE_KNIGHT, COLOR_WHITE };

    MoveList list = {};
    GenerateKnightMoves(gs, &list);

    if (list.count != 8) return false;
    if (!FindMove(&list, 3, 4, 5, 5)) return false;
    if (!FindMove(&list, 3, 4, 5, 3)) return false;
    if (!FindMove(&list, 3, 4, 1, 5)) return false;
    if (!FindMove(&list, 3, 4, 1, 3)) return false;
    if (!FindMove(&list, 3, 4, 4, 6)) return false;
    if (!FindMove(&list, 3, 4, 4, 2)) return false;
    if (!FindMove(&list, 3, 4, 2, 6)) return false;
    if (!FindMove(&list, 3, 4, 2, 2)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Knight in corner (a1 = rank 0, file 0) — expects only 2 moves.
// ---------------------------------------------------------------------------
static bool TestKnight_CornerMoves(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White knight on a1 (rank 0, file 0).
    gs->board.squares[0][0] = { PIECE_KNIGHT, COLOR_WHITE };

    MoveList list = {};
    GenerateKnightMoves(gs, &list);

    if (list.count != 2) return false;
    if (!FindMove(&list, 0, 0, 2, 1)) return false;
    if (!FindMove(&list, 0, 0, 1, 2)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Knight cannot capture a friendly piece.
// ---------------------------------------------------------------------------
static bool TestKnight_NoFriendlyCapture(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White knight on e4 (rank 3, file 4); white rook on f6 (rank 5, file 5).
    gs->board.squares[3][4] = { PIECE_KNIGHT, COLOR_WHITE };
    gs->board.squares[5][5] = { PIECE_ROOK,   COLOR_WHITE };

    MoveList list = {};
    GenerateKnightMoves(gs, &list);

    // f6 must not appear in the move list.
    if (FindMove(&list, 3, 4, 5, 5)) return false;
    // All other 7 squares should still be reachable.
    if (list.count != 7) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Knight can capture an enemy piece.
// ---------------------------------------------------------------------------
static bool TestKnight_EnemyCapture(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White knight on e4 (rank 3, file 4); black rook on f6 (rank 5, file 5).
    gs->board.squares[3][4] = { PIECE_KNIGHT, COLOR_WHITE };
    gs->board.squares[5][5] = { PIECE_ROOK,   COLOR_BLACK };

    MoveList list = {};
    GenerateKnightMoves(gs, &list);

    // f6 must appear — enemy capture is legal.
    if (!FindMove(&list, 3, 4, 5, 5)) return false;
    if (list.count != 8) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Rook on an open board — all rays extend to the board edge.
// ---------------------------------------------------------------------------
static bool TestRook_OpenBoard(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White rook on d4 (rank 3, file 3).
    gs->board.squares[3][3] = { PIECE_ROOK, COLOR_WHITE };

    MoveList list = {};
    GenerateRookMoves(gs, &list);

    // Along the file (rank axis): 3 squares up + 4 squares down = 7 moves.
    // Along the rank (file axis): 3 squares left + 4 squares right = 7 moves.
    if (list.count != 14) return false;

    // Spot check: all four directions.
    if (!FindMove(&list, 3, 3, 7, 3)) return false; // rank 7 (north)
    if (!FindMove(&list, 3, 3, 0, 3)) return false; // rank 0 (south)
    if (!FindMove(&list, 3, 3, 3, 7)) return false; // file 7 (east)
    if (!FindMove(&list, 3, 3, 3, 0)) return false; // file 0 (west)
    return true;
}

// ---------------------------------------------------------------------------
// Rook blocked by a friendly piece — ray stops before it.
// ---------------------------------------------------------------------------
static bool TestRook_BlockedByFriendly(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White rook on d4 (rank 3, file 3); white pawn on d6 (rank 5, file 3).
    gs->board.squares[3][3] = { PIECE_ROOK, COLOR_WHITE };
    gs->board.squares[5][3] = { PIECE_PAWN, COLOR_WHITE };

    MoveList list = {};
    GenerateRookMoves(gs, &list);

    // d5 (rank 4, file 3) is reachable; d6 (rank 5) is not.
    if (!FindMove(&list, 3, 3, 4, 3)) return false;
    if ( FindMove(&list, 3, 3, 5, 3)) return false;
    if ( FindMove(&list, 3, 3, 6, 3)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Rook captures an enemy piece — includes the capture square, stops beyond.
// ---------------------------------------------------------------------------
static bool TestRook_CapturesEnemy(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White rook on d4 (rank 3, file 3); black pawn on d6 (rank 5, file 3).
    gs->board.squares[3][3] = { PIECE_ROOK, COLOR_WHITE };
    gs->board.squares[5][3] = { PIECE_PAWN, COLOR_BLACK };

    MoveList list = {};
    GenerateRookMoves(gs, &list);

    // d5 and d6 (capture) are reachable; beyond d6 is not.
    if (!FindMove(&list, 3, 3, 4, 3)) return false;
    if (!FindMove(&list, 3, 3, 5, 3)) return false;
    if ( FindMove(&list, 3, 3, 6, 3)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Bishop on an open board — all four diagonal rays to board edges.
// ---------------------------------------------------------------------------
static bool TestBishop_OpenBoard(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White bishop on d4 (rank 3, file 3).
    gs->board.squares[3][3] = { PIECE_BISHOP, COLOR_WHITE };

    MoveList list = {};
    GenerateBishopMoves(gs, &list);

    // NE (+r,+f): steps 1-4 → rank 4-7, file 4-7 (4 squares)
    // NW (+r,-f): steps 1-3 → rank 4-6, file 2-0 (3 squares)
    // SE (-r,+f): steps 1-3 → rank 2-0, file 4-6 (3 squares)
    // SW (-r,-f): steps 1-3 → rank 2-0, file 2-0 (3 squares)
    // Total: 4+3+3+3 = 13
    if (list.count != 13) return false;

    // Spot-check one square per diagonal.
    if (!FindMove(&list, 3, 3, 7, 7)) return false; // NE corner
    if (!FindMove(&list, 3, 3, 6, 0)) return false; // NW
    if (!FindMove(&list, 3, 3, 0, 6)) return false; // SE
    if (!FindMove(&list, 3, 3, 0, 0)) return false; // SW corner
    return true;
}

// ---------------------------------------------------------------------------
// Bishop blocked by a friendly piece on one diagonal.
// ---------------------------------------------------------------------------
static bool TestBishop_BlockedByFriendly(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White bishop on d4 (rank 3, file 3); white pawn on f6 (rank 5, file 5).
    gs->board.squares[3][3] = { PIECE_BISHOP, COLOR_WHITE };
    gs->board.squares[5][5] = { PIECE_PAWN,   COLOR_WHITE };

    MoveList list = {};
    GenerateBishopMoves(gs, &list);

    // e5 (rank 4, file 4) is reachable on the NE diagonal; f6 is not.
    if (!FindMove(&list, 3, 3, 4, 4)) return false;
    if ( FindMove(&list, 3, 3, 5, 5)) return false;
    if ( FindMove(&list, 3, 3, 6, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Bishop captures an enemy on a diagonal — includes that square, stops beyond.
// ---------------------------------------------------------------------------
static bool TestBishop_CapturesEnemy(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White bishop on d4 (rank 3, file 3); black pawn on f6 (rank 5, file 5).
    gs->board.squares[3][3] = { PIECE_BISHOP, COLOR_WHITE };
    gs->board.squares[5][5] = { PIECE_PAWN,   COLOR_BLACK };

    MoveList list = {};
    GenerateBishopMoves(gs, &list);

    // e5 and f6 (capture) are reachable; g7 and beyond are not.
    if (!FindMove(&list, 3, 3, 4, 4)) return false;
    if (!FindMove(&list, 3, 3, 5, 5)) return false;
    if ( FindMove(&list, 3, 3, 6, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Queen on open board — all eight rays (rook + bishop combined).
// ---------------------------------------------------------------------------
static bool TestQueen_OpenBoard(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White queen on d4 (rank 3, file 3).
    gs->board.squares[3][3] = { PIECE_QUEEN, COLOR_WHITE };

    MoveList list = {};
    GenerateQueenMoves(gs, &list);

    // Orthogonal: 7+7 = 14; diagonal: 13 (same as bishop test above).
    if (list.count != 27) return false;

    // Spot-check a square in each of the eight directions.
    if (!FindMove(&list, 3, 3, 7, 3)) return false; // north
    if (!FindMove(&list, 3, 3, 0, 3)) return false; // south
    if (!FindMove(&list, 3, 3, 3, 7)) return false; // east
    if (!FindMove(&list, 3, 3, 3, 0)) return false; // west
    if (!FindMove(&list, 3, 3, 7, 7)) return false; // NE
    if (!FindMove(&list, 3, 3, 6, 0)) return false; // NW
    if (!FindMove(&list, 3, 3, 0, 6)) return false; // SE
    if (!FindMove(&list, 3, 3, 0, 0)) return false; // SW
    return true;
}

// ---------------------------------------------------------------------------
// Queen blocked by a friendly piece in one direction.
// ---------------------------------------------------------------------------
static bool TestQueen_BlockedByFriendly(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White queen on d4 (rank 3, file 3); white pawn on d6 (rank 5, file 3).
    gs->board.squares[3][3] = { PIECE_QUEEN, COLOR_WHITE };
    gs->board.squares[5][3] = { PIECE_PAWN,  COLOR_WHITE };

    MoveList list = {};
    GenerateQueenMoves(gs, &list);

    // d5 is reachable northward; d6 and beyond are not.
    if (!FindMove(&list, 3, 3, 4, 3)) return false;
    if ( FindMove(&list, 3, 3, 5, 3)) return false;
    if ( FindMove(&list, 3, 3, 6, 3)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Queen captures an enemy — includes the capture square, stops beyond.
// ---------------------------------------------------------------------------
static bool TestQueen_CapturesEnemy(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White queen on d4 (rank 3, file 3); black pawn on g7 (rank 6, file 6).
    gs->board.squares[3][3] = { PIECE_QUEEN, COLOR_WHITE };
    gs->board.squares[6][6] = { PIECE_PAWN,  COLOR_BLACK };

    MoveList list = {};
    GenerateQueenMoves(gs, &list);

    // e5, f6 are reachable; g7 is a capture (reachable); h8 is not.
    if (!FindMove(&list, 3, 3, 4, 4)) return false;
    if (!FindMove(&list, 3, 3, 5, 5)) return false;
    if (!FindMove(&list, 3, 3, 6, 6)) return false;
    if ( FindMove(&list, 3, 3, 7, 7)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: not in check — king on open board with no enemy pieces.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_NotInCheck(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e1 (rank 0, file 4), no enemy pieces.
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };

    if (IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: pawn check — enemy pawn diagonally in front of king.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_ByPawn(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e4 (rank 3, file 4); black pawn on f5 (rank 4, file 5).
    // A black pawn on f5 attacks e4 diagonally (black moves downward, attacks rank 3).
    gs->board.squares[3][4] = { PIECE_KING,  COLOR_WHITE };
    gs->board.squares[4][5] = { PIECE_PAWN,  COLOR_BLACK };

    if (!IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: knight check — enemy knight attacks the king's square.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_ByKnight(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e4 (rank 3, file 4); black knight on d6 (rank 5, file 3).
    // Knight on d6 attacks e4 via the (-2, +1) offset.
    gs->board.squares[3][4] = { PIECE_KING,   COLOR_WHITE };
    gs->board.squares[5][3] = { PIECE_KNIGHT, COLOR_BLACK };

    if (!IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: bishop check — enemy bishop on same diagonal.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_ByBishop(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e1 (rank 0, file 4); black bishop on b4 (rank 3, file 1).
    // They share the NW-SE diagonal.
    gs->board.squares[0][4] = { PIECE_KING,   COLOR_WHITE };
    gs->board.squares[3][1] = { PIECE_BISHOP, COLOR_BLACK };

    if (!IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: rook check — enemy rook on the same file.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_ByRook(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e1 (rank 0, file 4); black rook on e8 (rank 7, file 4).
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[7][4] = { PIECE_ROOK, COLOR_BLACK };

    if (!IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: queen check — enemy queen attacks along a rank.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_ByQueen(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e1 (rank 0, file 4); black queen on a1 (rank 0, file 0).
    gs->board.squares[0][4] = { PIECE_KING,  COLOR_WHITE };
    gs->board.squares[0][0] = { PIECE_QUEEN, COLOR_BLACK };

    if (!IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// IsInCheck: piece blocks the attack — king is not in check.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_BlockedByPiece(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e1 (rank 0, file 4); white pawn on e4 (rank 3, file 4)
    // blocking a black rook on e8 (rank 7, file 4).
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[3][4] = { PIECE_PAWN, COLOR_WHITE };
    gs->board.squares[7][4] = { PIECE_ROOK, COLOR_BLACK };

    if (IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

bool RunMovesTests(AppMemory* memory)
{
    ASSERT(memory);
    s_Memory = memory;

    RUN_TEST(TestPawn_WhiteDoublePush);
    RUN_TEST(TestPawn_BlackDoublePush);
    RUN_TEST(TestPawn_BlockedSinglePush);
    RUN_TEST(TestPawn_BlockedDoublePush);
    RUN_TEST(TestPawn_DiagonalCapture);
    RUN_TEST(TestPawn_NoFriendlyCapture);
    RUN_TEST(TestPawn_EnPassant);
    RUN_TEST(TestPawn_ApplyEnPassant);
    RUN_TEST(TestPawn_Promotion);
    RUN_TEST(TestPawn_ApplyPromotion);
    RUN_TEST(TestPawn_EnPassantTargetTracking);

    RUN_TEST(TestKnight_CenterMoves);
    RUN_TEST(TestKnight_CornerMoves);
    RUN_TEST(TestKnight_NoFriendlyCapture);
    RUN_TEST(TestKnight_EnemyCapture);

    RUN_TEST(TestRook_OpenBoard);
    RUN_TEST(TestRook_BlockedByFriendly);
    RUN_TEST(TestRook_CapturesEnemy);

    RUN_TEST(TestBishop_OpenBoard);
    RUN_TEST(TestBishop_BlockedByFriendly);
    RUN_TEST(TestBishop_CapturesEnemy);

    RUN_TEST(TestQueen_OpenBoard);
    RUN_TEST(TestQueen_BlockedByFriendly);
    RUN_TEST(TestQueen_CapturesEnemy);

    RUN_TEST(TestIsInCheck_NotInCheck);
    RUN_TEST(TestIsInCheck_ByPawn);
    RUN_TEST(TestIsInCheck_ByKnight);
    RUN_TEST(TestIsInCheck_ByBishop);
    RUN_TEST(TestIsInCheck_ByRook);
    RUN_TEST(TestIsInCheck_ByQueen);
    RUN_TEST(TestIsInCheck_BlockedByPiece);

    return true;
}
