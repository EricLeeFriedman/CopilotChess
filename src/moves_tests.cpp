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
// King in center (e4 = rank 3, file 4) — all 8 adjacent moves.
// ---------------------------------------------------------------------------
static bool TestKing_CenterMoves(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[3][4] = { PIECE_KING, COLOR_WHITE };

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    if (list.count != 8) return false;
    if (!FindMove(&list, 3, 4, 4, 4)) return false;
    if (!FindMove(&list, 3, 4, 4, 5)) return false;
    if (!FindMove(&list, 3, 4, 4, 3)) return false;
    if (!FindMove(&list, 3, 4, 3, 5)) return false;
    if (!FindMove(&list, 3, 4, 3, 3)) return false;
    if (!FindMove(&list, 3, 4, 2, 4)) return false;
    if (!FindMove(&list, 3, 4, 2, 5)) return false;
    if (!FindMove(&list, 3, 4, 2, 3)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// King in corner (a1 = rank 0, file 0) — only 3 adjacent moves.
// ---------------------------------------------------------------------------
static bool TestKing_CornerMoves(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE };
    // No castling rights since we cleared the board and king is not on e1.
    gs->castling_wk = false;
    gs->castling_wq = false;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    if (list.count != 3) return false;
    if (!FindMove(&list, 0, 0, 1, 0)) return false;
    if (!FindMove(&list, 0, 0, 1, 1)) return false;
    if (!FindMove(&list, 0, 0, 0, 1)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// King cannot capture a friendly piece.
// ---------------------------------------------------------------------------
static bool TestKing_NoFriendlyCapture(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[3][4] = { PIECE_KING,  COLOR_WHITE };
    gs->board.squares[4][4] = { PIECE_ROOK,  COLOR_WHITE }; // blocks one square

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // (4,4) must not be in the list.
    if (FindMove(&list, 3, 4, 4, 4)) return false;
    if (list.count != 7) return false;
    return true;
}

// ---------------------------------------------------------------------------
// King can capture an enemy piece that is not protected.
// ---------------------------------------------------------------------------
static bool TestKing_EnemyCapture(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e4 (3,4); isolated black pawn on f5 (4,5) — not protected.
    // A black pawn attacks diagonally toward rank 0, so it attacks (3,4) and (3,6),
    // not any other squares adjacent to the king.
    gs->board.squares[3][4] = { PIECE_KING,  COLOR_WHITE };
    gs->board.squares[4][5] = { PIECE_PAWN,  COLOR_BLACK };

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // King must be able to capture the pawn on f5 (4,5).
    // Note: (4,5) is attacked by the pawn's own attack pattern (black pawn
    // attacks from (4,5) toward (3,4) and (3,6)), but the pawn does NOT attack
    // its own square, so the king can capture it.
    if (!FindMove(&list, 3, 4, 4, 5)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// King cannot move to an attacked square.
// ---------------------------------------------------------------------------
static bool TestKing_CannotMoveIntoAttackedSquare(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e1 (0,4); black rook on e8 (7,4) controls the entire e-file.
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[7][4] = { PIECE_ROOK, COLOR_BLACK };

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // e2 (1,4) is on the e-file, attacked by the black rook — must not be in list.
    if (FindMove(&list, 0, 4, 1, 4)) return false;
    // d2 (1,3) and f2 (1,5) are not on the e-file, so they should be reachable.
    if (!FindMove(&list, 0, 4, 1, 3)) return false;
    if (!FindMove(&list, 0, 4, 1, 5)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Helper: find a castling move in list.
// ---------------------------------------------------------------------------
static bool FindCastlingMove(const MoveList* list, int8 fr, int8 ff, int8 tr, int8 tf)
{
    for (int32 i = 0; i < list->count; ++i)
    {
        const Move& m = list->moves[i];
        if (m.from_rank == fr && m.from_file == ff &&
            m.to_rank   == tr && m.to_file   == tf &&
            m.is_castling)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// King cannot move into a square attacked via X-ray through its own origin.
// White king on e1 (0,4); black rook on c1 (0,2) — the rook's eastward ray
// is blocked by the king at e1, but f1 (0,5) is on that ray past e1.
// The king must not be allowed to step to f1.
// ---------------------------------------------------------------------------
static bool TestKing_CannotMoveToXRayAttackedSquare(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // King on e1, black rook on c1.  The rook's east ray is: c1 → d1 → (e1 skipped
    // after king moves) → f1.  Without the fix the king would shield f1; with it f1
    // is correctly identified as attacked.
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][2] = { PIECE_ROOK, COLOR_BLACK };
    gs->castling_wk = false;
    gs->castling_wq = false;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // d1 (0,3) is directly attacked by the rook — must not be in list.
    if ( FindMove(&list, 0, 4, 0, 3)) return false;
    // f1 (0,5) is attacked via x-ray through e1 — must not be in list.
    if ( FindMove(&list, 0, 4, 0, 5)) return false;
    // d2 (1,3) and f2 (1,5) are not on the rook's ray — should be reachable.
    if (!FindMove(&list, 0, 4, 1, 3)) return false;
    if (!FindMove(&list, 0, 4, 1, 5)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling blocked by X-ray attack on transit square through king's origin.
// White king on e1 (0,4), white rook h1 (0,7), black rook b1 (0,1).
// The black rook's eastward ray passes through the vacated e1 to reach f1,
// so kingside castling must be refused.
// ---------------------------------------------------------------------------
static bool TestCastling_XRayBlocksTransit(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };
    gs->board.squares[0][1] = { PIECE_ROOK, COLOR_BLACK }; // b1: attacks e1 ray east
    gs->castling_wk = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // f1 (0,5) is the transit square — attacked via x-ray; castling must be refused.
    if (FindCastlingMove(&list, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// White kingside castling — all conditions met.
// ---------------------------------------------------------------------------
static bool TestCastling_WhiteKingside(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };
    gs->castling_wk = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // Castling move: king from e1 (0,4) to g1 (0,6).
    if (!FindCastlingMove(&list, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// White queenside castling — all conditions met.
// ---------------------------------------------------------------------------
static bool TestCastling_WhiteQueenside(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][0] = { PIECE_ROOK, COLOR_WHITE };
    gs->castling_wq = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // Castling move: king from e1 (0,4) to c1 (0,2).
    if (!FindCastlingMove(&list, 0, 4, 0, 2)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black kingside castling — all conditions met.
// ---------------------------------------------------------------------------
static bool TestCastling_BlackKingside(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[7][4] = { PIECE_KING, COLOR_BLACK };
    gs->board.squares[7][7] = { PIECE_ROOK, COLOR_BLACK };
    gs->side_to_move = COLOR_BLACK;
    gs->castling_bk  = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    // Castling move: king from e8 (7,4) to g8 (7,6).
    if (!FindCastlingMove(&list, 7, 4, 7, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling blocked: piece standing between king and rook.
// ---------------------------------------------------------------------------
static bool TestCastling_BlockedByPiece(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING,   COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK,   COLOR_WHITE };
    gs->board.squares[0][6] = { PIECE_BISHOP, COLOR_WHITE }; // blocks kingside path
    gs->castling_wk = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    if (FindCastlingMove(&list, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling blocked: king passes through an attacked square.
// ---------------------------------------------------------------------------
static bool TestCastling_ThroughAttackedSquare(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };
    // Black rook on f8 (rank 7, file 5) attacks f1 (rank 0, file 5) — the transit square.
    gs->board.squares[7][5] = { PIECE_ROOK, COLOR_BLACK };
    gs->castling_wk = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    if (FindCastlingMove(&list, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling blocked: the destination square is attacked.
// ---------------------------------------------------------------------------
static bool TestCastling_LandingSquareAttacked(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };
    // Black rook on g8 (rank 7, file 6) attacks g1 (rank 0, file 6) — the landing square.
    gs->board.squares[7][6] = { PIECE_ROOK, COLOR_BLACK };
    gs->castling_wk = true;

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    if (FindCastlingMove(&list, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling blocked: castling right has been revoked.
// ---------------------------------------------------------------------------
static bool TestCastling_NoRight(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };
    gs->castling_wk = false; // right explicitly cleared

    MoveList list = {};
    GenerateKingMoves(gs, &list);

    if (FindCastlingMove(&list, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// ApplyMove for kingside castling moves the rook from h1 to f1.
// ---------------------------------------------------------------------------
static bool TestCastling_ApplyKingside(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };

    Move m         = {};
    m.from_rank    = 0; m.from_file    = 4;
    m.to_rank      = 0; m.to_file      = 6;
    m.promotion    = PIECE_NONE;
    m.is_en_passant = false;
    m.is_castling   = true;

    ApplyMove(gs, &m);

    // King must be on g1 (0,6); rook on f1 (0,5); h1 and e1 empty.
    if (gs->board.squares[0][6].piece != PIECE_KING)  return false;
    if (gs->board.squares[0][6].color != COLOR_WHITE) return false;
    if (gs->board.squares[0][5].piece != PIECE_ROOK)  return false;
    if (gs->board.squares[0][5].color != COLOR_WHITE) return false;
    if (gs->board.squares[0][7].piece != PIECE_NONE)  return false;
    if (gs->board.squares[0][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// ApplyMove for queenside castling moves the rook from a1 to d1.
// ---------------------------------------------------------------------------
static bool TestCastling_ApplyQueenside(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][0] = { PIECE_ROOK, COLOR_WHITE };

    Move m         = {};
    m.from_rank    = 0; m.from_file    = 4;
    m.to_rank      = 0; m.to_file      = 2;
    m.promotion    = PIECE_NONE;
    m.is_en_passant = false;
    m.is_castling   = true;

    ApplyMove(gs, &m);

    // King must be on c1 (0,2); rook on d1 (0,3); a1 and e1 empty.
    if (gs->board.squares[0][2].piece != PIECE_KING)  return false;
    if (gs->board.squares[0][2].color != COLOR_WHITE) return false;
    if (gs->board.squares[0][3].piece != PIECE_ROOK)  return false;
    if (gs->board.squares[0][3].color != COLOR_WHITE) return false;
    if (gs->board.squares[0][0].piece != PIECE_NONE)  return false;
    if (gs->board.squares[0][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling rights revoked after king moves.
// ---------------------------------------------------------------------------
static bool TestCastling_RightsRevokedByKingMove(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };

    Move m         = {};
    m.from_rank    = 0; m.from_file    = 4;
    m.to_rank      = 0; m.to_file      = 5; // non-castling king move
    m.promotion    = PIECE_NONE;
    m.is_en_passant = false;
    m.is_castling   = false;

    ApplyMove(gs, &m);

    if (gs->castling_wk) return false;
    if (gs->castling_wq) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling rights revoked after the h1 rook moves (white kingside).
// ---------------------------------------------------------------------------
static bool TestCastling_RightsRevokedByRookMove(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE };

    Move m         = {};
    m.from_rank    = 0; m.from_file    = 7;
    m.to_rank      = 3; m.to_file      = 7;
    m.promotion    = PIECE_NONE;
    m.is_en_passant = false;
    m.is_castling   = false;

    ApplyMove(gs, &m);

    if (gs->castling_wk)  return false; // kingside right must be gone
    if (!gs->castling_wq) return false; // queenside right untouched
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

    RUN_TEST(TestKing_CenterMoves);
    RUN_TEST(TestKing_CornerMoves);
    RUN_TEST(TestKing_NoFriendlyCapture);
    RUN_TEST(TestKing_EnemyCapture);
    RUN_TEST(TestKing_CannotMoveIntoAttackedSquare);
    RUN_TEST(TestKing_CannotMoveToXRayAttackedSquare);

    RUN_TEST(TestCastling_WhiteKingside);
    RUN_TEST(TestCastling_WhiteQueenside);
    RUN_TEST(TestCastling_BlackKingside);
    RUN_TEST(TestCastling_BlockedByPiece);
    RUN_TEST(TestCastling_ThroughAttackedSquare);
    RUN_TEST(TestCastling_LandingSquareAttacked);
    RUN_TEST(TestCastling_NoRight);
    RUN_TEST(TestCastling_XRayBlocksTransit);
    RUN_TEST(TestCastling_ApplyKingside);
    RUN_TEST(TestCastling_ApplyQueenside);
    RUN_TEST(TestCastling_RightsRevokedByKingMove);
    RUN_TEST(TestCastling_RightsRevokedByRookMove);

    return true;
}
