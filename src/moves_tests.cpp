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

// Same as FindMove but also requires is_castling flag.
static bool FindCastlingMove(const MoveList* list, int8 fr, int8 ff, int8 tr, int8 tf)
{
    for (int32 i = 0; i < list->count; ++i)
    {
        const Move& m = list->moves[i];
        if (m.from_rank == fr && m.from_file == ff &&
            m.to_rank   == tr && m.to_file   == tf &&
            m.is_castling)
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

// ---------------------------------------------------------------------------
// IsInCheck: adjacent enemy king attacks — kings next to each other.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_ByKing(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e4 (rank 3, file 4); black king on f5 (rank 4, file 5) — adjacent.
    gs->board.squares[3][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[4][5] = { PIECE_KING, COLOR_BLACK };

    if (!IsInCheck(&gs->board, COLOR_WHITE)) return false;
    if (!IsInCheck(&gs->board, COLOR_BLACK)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// GetLegalMoves: pinned rook cannot move off the pin ray.
// White king on e1 (rank 0, file 4), white rook on e4 (rank 3, file 4),
// black rook on e8 (rank 7, file 4).  The white rook is pinned along the
// e-file; any move to a different file would expose the king.
// ---------------------------------------------------------------------------
static bool TestGetLegalMoves_PinnedRook(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING,  COLOR_WHITE };
    gs->board.squares[3][4] = { PIECE_ROOK,  COLOR_WHITE };
    gs->board.squares[7][4] = { PIECE_ROOK,  COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // White rook may only move along the e-file (ranks 1-2 toward king side
    // are blocked by king, but rank-wise moves up to the enemy rook are fine).
    // It must NOT appear at any non-e-file square in the legal list.
    for (int32 i = 0; i < legal.count; ++i)
    {
        const Move& m = legal.moves[i];
        if (m.from_rank == 3 && m.from_file == 4)
        {
            // Rook move — must stay on file 4.
            if (m.to_file != 4) return false;
        }
    }

    // The rook should be able to capture on e8 (rank 7, file 4).
    if (!FindMove(&legal, 3, 4, 7, 4)) return false;
    // The rook should NOT be able to move to, say, d4 (rank 3, file 3).
    if (FindMove(&legal, 3, 4, 3, 3)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// GetLegalMoves: king cannot walk into check.
// White king on e1 (rank 0, file 4), black rook on f8 (rank 7, file 5).
// The f-file is controlled by the rook, so the king must not step to f1 or f2.
// ---------------------------------------------------------------------------
static bool TestGetLegalMoves_KingCannotWalkIntoCheck(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[7][5] = { PIECE_ROOK, COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // King must not move onto file 5 (f-file) — controlled by the black rook.
    if (FindMove(&legal, 0, 4, 0, 5)) return false; // f1
    if (FindMove(&legal, 0, 4, 1, 5)) return false; // f2
    // King may still move to d1, d2, e2.
    if (!FindMove(&legal, 0, 4, 0, 3)) return false; // d1
    if (!FindMove(&legal, 0, 4, 1, 3)) return false; // d2
    if (!FindMove(&legal, 0, 4, 1, 4)) return false; // e2
    return true;
}

// ---------------------------------------------------------------------------
// GetLegalMoves: pinned knight has no legal moves.
// White king on e1 (rank 0, file 4), white knight on e3 (rank 2, file 4),
// black rook on e8 (rank 7, file 4).  The knight is on the e-file between
// king and rook; moving it in any L-shape exposes the king.
// ---------------------------------------------------------------------------
static bool TestGetLegalMoves_PinnedKnightHasNoMoves(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING,   COLOR_WHITE };
    gs->board.squares[2][4] = { PIECE_KNIGHT, COLOR_WHITE };
    gs->board.squares[7][4] = { PIECE_ROOK,   COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // No knight move should appear in the legal list.
    for (int32 i = 0; i < legal.count; ++i)
    {
        if (legal.moves[i].from_rank == 2 && legal.moves[i].from_file == 4)
            return false;
    }
    return true;
}


// ---------------------------------------------------------------------------
// Castling: white kingside castling available when path is clear.
// ---------------------------------------------------------------------------
static bool TestCastling_WhiteKingsideAvailable(void)
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
    gs->board.squares[7][4] = { PIECE_KING, COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // Kingside castling: king from e1 (0,4) to g1 (0,6).
    if (!FindCastlingMove(&legal, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling: white queenside castling available when path is clear.
// ---------------------------------------------------------------------------
static bool TestCastling_WhiteQueensideAvailable(void)
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
    gs->board.squares[7][4] = { PIECE_KING, COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // Queenside castling: king from e1 (0,4) to c1 (0,2).
    if (!FindCastlingMove(&legal, 0, 4, 0, 2)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling: kingside castling blocked by a piece on f1.
// ---------------------------------------------------------------------------
static bool TestCastling_KingsideBlockedByPiece(void)
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
    gs->board.squares[0][5] = { PIECE_BISHOP, COLOR_WHITE }; // f1 blocks path

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // Kingside castling must not appear.
    if (FindCastlingMove(&legal, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling: kingside castling forbidden when the transit square (f1) is attacked.
// ---------------------------------------------------------------------------
static bool TestCastling_KingsideThroughCheck(void)
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
    // Black rook on f8 (rank 7, file 5) controls the entire f-file.
    gs->board.squares[7][5] = { PIECE_ROOK, COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // Kingside castling must not appear because f1 is attacked.
    if (FindCastlingMove(&legal, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling: ApplyMove — after kingside castle, king on g1, rook on f1.
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

    Move cm          = {};
    cm.from_rank     = 0; cm.from_file = 4; // e1
    cm.to_rank       = 0; cm.to_file   = 6; // g1
    cm.promotion     = PIECE_NONE;
    cm.is_en_passant = false;
    cm.is_castling   = true;

    ApplyMove(gs, &cm);

    // King should be on g1.
    if (gs->board.squares[0][6].piece != PIECE_KING) return false;
    if (gs->board.squares[0][6].color != COLOR_WHITE) return false;
    // Rook should be on f1.
    if (gs->board.squares[0][5].piece != PIECE_ROOK) return false;
    if (gs->board.squares[0][5].color != COLOR_WHITE) return false;
    // Old king square (e1) must be empty.
    if (gs->board.squares[0][4].piece != PIECE_NONE) return false;
    // Old rook square (h1) must be empty.
    if (gs->board.squares[0][7].piece != PIECE_NONE) return false;
    // Both white castling rights must now be cleared.
    if (gs->castling_white_kingside)  return false;
    if (gs->castling_white_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling: ApplyMove — after queenside castle, king on c1, rook on d1.
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

    Move cm          = {};
    cm.from_rank     = 0; cm.from_file = 4; // e1
    cm.to_rank       = 0; cm.to_file   = 2; // c1
    cm.promotion     = PIECE_NONE;
    cm.is_en_passant = false;
    cm.is_castling   = true;

    ApplyMove(gs, &cm);

    // King should be on c1.
    if (gs->board.squares[0][2].piece != PIECE_KING) return false;
    if (gs->board.squares[0][2].color != COLOR_WHITE) return false;
    // Rook should be on d1.
    if (gs->board.squares[0][3].piece != PIECE_ROOK) return false;
    if (gs->board.squares[0][3].color != COLOR_WHITE) return false;
    // Old king square (e1) must be empty.
    if (gs->board.squares[0][4].piece != PIECE_NONE) return false;
    // Old rook square (a1) must be empty.
    if (gs->board.squares[0][0].piece != PIECE_NONE) return false;
    // Both white castling rights must now be cleared.
    if (gs->castling_white_kingside)  return false;
    if (gs->castling_white_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling rights: both white rights cleared when white king moves.
// ---------------------------------------------------------------------------
static bool TestCastling_RightsLostAfterKingMove(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };

    Move km          = {};
    km.from_rank     = 0; km.from_file = 4; // e1
    km.to_rank       = 1; km.to_file   = 4; // e2
    km.promotion     = PIECE_NONE;
    km.is_en_passant = false;
    km.is_castling   = false;

    ApplyMove(gs, &km);

    if (gs->castling_white_kingside)  return false;
    if (gs->castling_white_queenside) return false;
    // Black rights must be unaffected.
    if (!gs->castling_black_kingside)  return false;
    if (!gs->castling_black_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling rights: only the kingside right cleared when the h1-rook moves.
// ---------------------------------------------------------------------------
static bool TestCastling_RightsLostAfterRookMove(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE }; // h1
    gs->board.squares[0][0] = { PIECE_ROOK, COLOR_WHITE }; // a1

    // Move the h1-rook away.
    Move rm          = {};
    rm.from_rank     = 0; rm.from_file = 7; // h1
    rm.to_rank       = 3; rm.to_file   = 7; // h4
    rm.promotion     = PIECE_NONE;
    rm.is_en_passant = false;
    rm.is_castling   = false;

    ApplyMove(gs, &rm);

    // Kingside right must be gone; queenside right must remain.
    if (gs->castling_white_kingside)   return false;
    if (!gs->castling_white_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Castling rights: white kingside right cleared when h1-rook is captured.
// ---------------------------------------------------------------------------
static bool TestCastling_RightsLostAfterRookCaptured(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_WHITE }; // h1 (target of capture)
    gs->board.squares[7][7] = { PIECE_ROOK, COLOR_BLACK }; // h8, will capture h1 rook

    gs->side_to_move = COLOR_BLACK;

    // Black rook captures the white h1-rook.
    Move cap         = {};
    cap.from_rank    = 7; cap.from_file = 7; // h8
    cap.to_rank      = 0; cap.to_file   = 7; // h1 (captures white rook)
    cap.promotion    = PIECE_NONE;
    cap.is_en_passant = false;
    cap.is_castling  = false;

    ApplyMove(gs, &cap);

    // White kingside right must be gone after its rook was captured.
    if (gs->castling_white_kingside) return false;
    // White queenside right should be unaffected.
    if (!gs->castling_white_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Regression: kingside castling is illegal when a black pawn on f2 attacks
// both the king's starting square (e1) and its destination (g1) diagonally.
// This verifies pawn attacks are detected correctly (diagonal only).
// ---------------------------------------------------------------------------
static bool TestCastling_KingsideIllegalWhenPawnAttacksStartAndEnd(void)
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
    // Black pawn on f2 (rank 1, file 5): diagonally attacks e1 (king start) and g1
    // (king destination). Kingside castling must be rejected.
    gs->board.squares[1][5] = { PIECE_PAWN, COLOR_BLACK };

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // Kingside castling must NOT appear — pawn attacks both e1 and g1.
    if (FindCastlingMove(&legal, 0, 4, 0, 6)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Regression: a pawn directly in front of the king (on the same file) does
// NOT put the king in check. Only diagonal pawn attacks count.
// A black pawn on e5 pushes toward e4 but only attacks d4 and f4 diagonally.
// ---------------------------------------------------------------------------
static bool TestIsInCheck_PawnDirectlyInFrontIsNotCheck(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // White king on e4 (rank 3, file 4); black pawn directly in front on e5
    // (rank 4, file 4). The pawn's forward push targets e4, but it ATTACKS
    // d4 and f4 only — NOT e4. So the king must NOT be in check.
    gs->board.squares[3][4] = { PIECE_KING, COLOR_WHITE };
    gs->board.squares[4][4] = { PIECE_PAWN, COLOR_BLACK };

    if (IsInCheck(&gs->board, COLOR_WHITE)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Regression: ApplyMove on a normal king step (is_castling = false) must NOT
// move the rook, even when a rook sits on h1.
// ---------------------------------------------------------------------------
static bool TestCastling_NormalKingMoveDoesNotMovRook(void)
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

    // Normal king move: e1 -> d1  (is_castling explicitly false)
    Move km          = {};
    km.from_rank     = 0; km.from_file = 4; // e1
    km.to_rank       = 0; km.to_file   = 3; // d1
    km.promotion     = PIECE_NONE;
    km.is_en_passant = false;
    km.is_castling   = false;

    ApplyMove(gs, &km);

    // King should be on d1.
    if (gs->board.squares[0][3].piece != PIECE_KING) return false;
    // Rook must still be on h1 — not moved.
    if (gs->board.squares[0][7].piece != PIECE_ROOK) return false;
    if (gs->board.squares[0][7].color != COLOR_WHITE) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Regression: an absolutely pinned enemy knight still attacks squares it
// cannot legally move to.  Per FIDE Article 3.8, a piece attacks its normal
// squares even when constrained from moving there by an absolute pin.
//
// Setup:
//   White king on e1 (rank 0, file 4), White rook on d1 (rank 0, file 3).
//   Black knight on d4 (rank 3, file 3), Black king on d8 (rank 7, file 3).
//
// The Black knight is absolutely pinned along the d-file (White rook on d1
// would attack Black king on d8 if the knight left d4).  The knight attacks
// e2 (rank 1, file 4); under FIDE rules that square is controlled, so the
// White king must NOT be allowed to step there.
// ---------------------------------------------------------------------------
static bool TestGetLegalMoves_PinnedEnemyKnightStillBlocksKingStep(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING,   COLOR_WHITE }; // e1
    gs->board.squares[0][3] = { PIECE_ROOK,   COLOR_WHITE }; // d1 — pins black knight
    gs->board.squares[3][3] = { PIECE_KNIGHT, COLOR_BLACK }; // d4 — absolutely pinned
    gs->board.squares[7][3] = { PIECE_KING,   COLOR_BLACK }; // d8

    // The pinned black knight on d4 attacks e2 (rank 1, file 4) per FIDE rules.
    // GetLegalMoves must NOT include the king step e1 -> e2.
    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    for (int32 i = 0; i < legal.count; ++i)
    {
        const Move& m = legal.moves[i];
        if (m.from_rank == 0 && m.from_file == 4 &&
            m.to_rank   == 1 && m.to_file   == 4 &&
            !m.is_castling)
        {
            return false; // king step to e2 must be absent
        }
    }

    // Also verify IsInCheck correctly reports check after a king move to e2.
    GameState after = *gs;
    Move km          = {};
    km.from_rank     = 0; km.from_file = 4;
    km.to_rank       = 1; km.to_file   = 4;
    km.promotion     = PIECE_NONE;
    km.is_en_passant = false;
    km.is_castling   = false;
    ApplyMove(&after, &km);
    if (!IsInCheck(&after.board, COLOR_WHITE)) return false; // must be in check

    return true;
}

// ---------------------------------------------------------------------------
// Regression: GetLegalMoves must never return a move that captures the
// opponent's king.  King capture is not a legal chess move; such positions
// must be handled through check/checkmate detection before they arise.
//
// Setup:
//   White queen on d1, Black king on d8, White king on e1.
//   White is to move.  The queen can pseudo-legally slide to d8 and "capture"
//   the Black king, but GetLegalMoves must not include that move.
// ---------------------------------------------------------------------------
static bool TestGetLegalMoves_CannotCaptureKing(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][3] = { PIECE_QUEEN, COLOR_WHITE }; // d1
    gs->board.squares[0][4] = { PIECE_KING,  COLOR_WHITE }; // e1
    gs->board.squares[7][3] = { PIECE_KING,  COLOR_BLACK }; // d8 — would be "captured"

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // d1 -> d8 must not be present.
    for (int32 i = 0; i < legal.count; ++i)
    {
        const Move& m = legal.moves[i];
        if (m.from_rank == 0 && m.from_file == 3 &&
            m.to_rank   == 7 && m.to_file   == 3)
        {
            return false; // king capture must be absent
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Regression: queenside castling must be illegal when an enemy rook on the
// h-file would attack the transit/destination squares once the king vacates
// its starting square.
//
// Setup:
//   White king on e1 (rank 0, file 4), White rook on a1 (rank 0, file 0).
//   Black rook on h1 (rank 0, file 7), Black king on e8 (rank 7, file 4).
//
// With the White king on e1, the Black rook on h1 attacks g1, f1, and is
// blocked by the king on e1.  Once the king moves away (as it does during
// queenside castling), the rook would attack d1 and c1 — the transit and
// destination squares.  GenerateCastlingMoves must detect this by checking
// those squares on a board with the king removed from e1.
// ---------------------------------------------------------------------------
static bool TestCastling_QueensideIllegalWhenRookAttacksThroughKingSquare(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE }; // e1
    gs->board.squares[0][0] = { PIECE_ROOK, COLOR_WHITE }; // a1 (queenside castling rook)
    gs->board.squares[0][7] = { PIECE_ROOK, COLOR_BLACK }; // h1 — attacks d1/c1 once king leaves e1
    gs->board.squares[7][4] = { PIECE_KING, COLOR_BLACK }; // e8

    gs->castling_white_queenside = true;
    gs->castling_white_kingside  = false;

    MoveList legal = {};
    GetLegalMoves(gs, &legal);

    // Queenside castling (e1 -> c1) must NOT appear in legal moves.
    for (int32 i = 0; i < legal.count; ++i)
    {
        const Move& m = legal.moves[i];
        if (m.is_castling && m.from_file == 4 && m.to_file == 2)
            return false; // illegal castle must be absent
    }
    return true;
}

// ---------------------------------------------------------------------------
// EvaluatePosition: initial position is ongoing.
// ---------------------------------------------------------------------------
static bool TestEvaluatePosition_Ongoing(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    if (EvaluatePosition(gs) != GAME_ONGOING) return false;
    return true;
}

// ---------------------------------------------------------------------------
// EvaluatePosition: Fool's mate — black wins by checkmate in 4 half-moves.
//   1. f3  e5
//   2. g4  Qh4#
// After these moves White is in check with no legal escape: GAME_BLACK_WINS.
// ---------------------------------------------------------------------------
static bool TestEvaluatePosition_Checkmate_FoolsMate(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // 1. f3 (f2 -> f3): rank 1, file 5 -> rank 2, file 5
    Move m1 = {};
    m1.from_rank = 1; m1.from_file = 5;
    m1.to_rank   = 2; m1.to_file   = 5;
    ApplyMove(gs, &m1);

    // 1...e5 (e7 -> e5): rank 6, file 4 -> rank 4, file 4
    Move m2 = {};
    m2.from_rank = 6; m2.from_file = 4;
    m2.to_rank   = 4; m2.to_file   = 4;
    ApplyMove(gs, &m2);

    // 2. g4 (g2 -> g4): rank 1, file 6 -> rank 3, file 6
    Move m3 = {};
    m3.from_rank = 1; m3.from_file = 6;
    m3.to_rank   = 3; m3.to_file   = 6;
    ApplyMove(gs, &m3);

    // 2...Qh4# (Qd8 -> h4): rank 7, file 3 -> rank 3, file 7
    Move m4 = {};
    m4.from_rank = 7; m4.from_file = 3;
    m4.to_rank   = 3; m4.to_file   = 7;
    ApplyMove(gs, &m4);

    // White is now in checkmate: Black wins.
    if (EvaluatePosition(gs) != GAME_BLACK_WINS) return false;
    return true;
}

// ---------------------------------------------------------------------------
// EvaluatePosition: classic stalemate — Black has no legal moves and is not
// in check.
//   White: King c6 (rank 5, file 2), Queen b6 (rank 5, file 1)
//   Black: King a8 (rank 7, file 0)
//   Side to move: BLACK
// Black king cannot move to a7 (attacked by queen), b8 (attacked by queen),
// or b7 (attacked by queen and king).  Not in check.  Result: GAME_DRAW.
// ---------------------------------------------------------------------------
static bool TestEvaluatePosition_Stalemate(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // Clear all squares.
    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[7][0] = { PIECE_KING,  COLOR_BLACK }; // Black king a8
    gs->board.squares[5][2] = { PIECE_KING,  COLOR_WHITE }; // White king c6
    gs->board.squares[5][1] = { PIECE_QUEEN, COLOR_WHITE }; // White queen b6

    gs->side_to_move             = COLOR_BLACK;
    gs->castling_white_kingside  = false;
    gs->castling_white_queenside = false;
    gs->castling_black_kingside  = false;
    gs->castling_black_queenside = false;

    if (EvaluatePosition(gs) != GAME_DRAW) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Turn management: side_to_move alternates after each move.
// ---------------------------------------------------------------------------
static bool TestApplyMove_TurnAlternates(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    // White to move initially.
    if (gs->side_to_move != COLOR_WHITE) return false;

    // White moves e2 -> e4.
    Move m1 = {};
    m1.from_rank = 1; m1.from_file = 4;
    m1.to_rank   = 3; m1.to_file   = 4;
    m1.promotion = PIECE_NONE;
    m1.is_en_passant = false;
    m1.is_castling = false;
    ApplyMove(gs, &m1);

    // Black to move after White's move.
    if (gs->side_to_move != COLOR_BLACK) return false;

    // Black moves e7 -> e5.
    Move m2 = {};
    m2.from_rank = 6; m2.from_file = 4;
    m2.to_rank   = 4; m2.to_file   = 4;
    m2.promotion = PIECE_NONE;
    m2.is_en_passant = false;
    m2.is_castling = false;
    ApplyMove(gs, &m2);

    // White to move after Black's move.
    if (gs->side_to_move != COLOR_WHITE) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black castling: ApplyMove — after kingside castle, king on g8, rook on f8.
// ---------------------------------------------------------------------------
static bool TestCastling_ApplyKingside_Black(void)
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
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE }; // White king on e1 (non-interfering)
    gs->side_to_move = COLOR_BLACK;

    Move cm          = {};
    cm.from_rank     = 7; cm.from_file = 4; // e8
    cm.to_rank       = 7; cm.to_file   = 6; // g8
    cm.promotion     = PIECE_NONE;
    cm.is_en_passant = false;
    cm.is_castling   = true;

    ApplyMove(gs, &cm);

    // King should be on g8.
    if (gs->board.squares[7][6].piece != PIECE_KING) return false;
    if (gs->board.squares[7][6].color != COLOR_BLACK) return false;
    // Rook should be on f8.
    if (gs->board.squares[7][5].piece != PIECE_ROOK) return false;
    if (gs->board.squares[7][5].color != COLOR_BLACK) return false;
    // Old king square (e8) must be empty.
    if (gs->board.squares[7][4].piece != PIECE_NONE) return false;
    // Old rook square (h8) must be empty.
    if (gs->board.squares[7][7].piece != PIECE_NONE) return false;
    // Both black castling rights must now be cleared.
    if (gs->castling_black_kingside)  return false;
    if (gs->castling_black_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black castling: ApplyMove — after queenside castle, king on c8, rook on d8.
// ---------------------------------------------------------------------------
static bool TestCastling_ApplyQueenside_Black(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[7][4] = { PIECE_KING, COLOR_BLACK };
    gs->board.squares[7][0] = { PIECE_ROOK, COLOR_BLACK };
    gs->board.squares[0][4] = { PIECE_KING, COLOR_WHITE }; // White king on e1 (non-interfering)
    gs->side_to_move = COLOR_BLACK;

    Move cm          = {};
    cm.from_rank     = 7; cm.from_file = 4; // e8
    cm.to_rank       = 7; cm.to_file   = 2; // c8
    cm.promotion     = PIECE_NONE;
    cm.is_en_passant = false;
    cm.is_castling   = true;

    ApplyMove(gs, &cm);

    // King should be on c8.
    if (gs->board.squares[7][2].piece != PIECE_KING) return false;
    if (gs->board.squares[7][2].color != COLOR_BLACK) return false;
    // Rook should be on d8.
    if (gs->board.squares[7][3].piece != PIECE_ROOK) return false;
    if (gs->board.squares[7][3].color != COLOR_BLACK) return false;
    // Old king square (e8) must be empty.
    if (gs->board.squares[7][4].piece != PIECE_NONE) return false;
    // Old rook square (a8) must be empty.
    if (gs->board.squares[7][0].piece != PIECE_NONE) return false;
    // Both black castling rights must now be cleared.
    if (gs->castling_black_kingside)  return false;
    if (gs->castling_black_queenside) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black en passant: ApplyMove removes the White pawn that was jumped over.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyEnPassant_Black(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // Black pawn on d4 (rank 3, file 3), White pawn on e4 (rank 3, file 4).
    gs->board.squares[3][3] = { PIECE_PAWN, COLOR_BLACK };
    gs->board.squares[3][4] = { PIECE_PAWN, COLOR_WHITE };
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->en_passant_rank = 2;
    gs->en_passant_file = 4;
    gs->side_to_move = COLOR_BLACK;

    Move ep    = {};
    ep.from_rank     = 3; ep.from_file     = 3;
    ep.to_rank       = 2; ep.to_file       = 4;
    ep.is_en_passant = true;
    ep.promotion     = PIECE_NONE;

    ApplyMove(gs, &ep);

    // Black pawn should now be on e3 (rank 2, file 4).
    if (gs->board.squares[2][4].piece != PIECE_PAWN)  return false;
    if (gs->board.squares[2][4].color != COLOR_BLACK) return false;
    // White pawn on e4 must be gone.
    if (gs->board.squares[3][4].piece != PIECE_NONE)  return false;
    // Black pawn's old square must be clear.
    if (gs->board.squares[3][3].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black promotion: ApplyMove replaces pawn with queen on rank 1.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_Black_Queen(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    // Black pawn on e2 (rank 1, file 4).
    gs->board.squares[1][4] = { PIECE_PAWN, COLOR_BLACK };
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->side_to_move = COLOR_BLACK;

    Move pm    = {};
    pm.from_rank  = 1; pm.from_file  = 4;
    pm.to_rank    = 0; pm.to_file    = 4;
    pm.promotion  = PIECE_QUEEN;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    // e1 should now contain a black queen.
    if (gs->board.squares[0][4].piece != PIECE_QUEEN) return false;
    if (gs->board.squares[0][4].color != COLOR_BLACK) return false;
    // e2 must be clear.
    if (gs->board.squares[1][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black promotion: ApplyMove replaces pawn with rook on rank 1.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_Black_Rook(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[1][4] = { PIECE_PAWN, COLOR_BLACK };
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->side_to_move = COLOR_BLACK;

    Move pm    = {};
    pm.from_rank  = 1; pm.from_file  = 4;
    pm.to_rank    = 0; pm.to_file    = 4;
    pm.promotion  = PIECE_ROOK;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    if (gs->board.squares[0][4].piece != PIECE_ROOK) return false;
    if (gs->board.squares[0][4].color != COLOR_BLACK) return false;
    if (gs->board.squares[1][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black promotion: ApplyMove replaces pawn with bishop on rank 1.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_Black_Bishop(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[1][4] = { PIECE_PAWN, COLOR_BLACK };
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->side_to_move = COLOR_BLACK;

    Move pm    = {};
    pm.from_rank  = 1; pm.from_file  = 4;
    pm.to_rank    = 0; pm.to_file    = 4;
    pm.promotion  = PIECE_BISHOP;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    if (gs->board.squares[0][4].piece != PIECE_BISHOP) return false;
    if (gs->board.squares[0][4].color != COLOR_BLACK) return false;
    if (gs->board.squares[1][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// Black promotion: ApplyMove replaces pawn with knight on rank 1.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_Black_Knight(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[1][4] = { PIECE_PAWN, COLOR_BLACK };
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->side_to_move = COLOR_BLACK;

    Move pm    = {};
    pm.from_rank  = 1; pm.from_file  = 4;
    pm.to_rank    = 0; pm.to_file    = 4;
    pm.promotion  = PIECE_KNIGHT;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    if (gs->board.squares[0][4].piece != PIECE_KNIGHT) return false;
    if (gs->board.squares[0][4].color != COLOR_BLACK) return false;
    if (gs->board.squares[1][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// White promotion: ApplyMove replaces pawn with rook on rank 8.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_White_Rook(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[6][4] = { PIECE_PAWN, COLOR_WHITE };
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)

    Move pm    = {};
    pm.from_rank  = 6; pm.from_file  = 4;
    pm.to_rank    = 7; pm.to_file    = 4;
    pm.promotion  = PIECE_ROOK;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    if (gs->board.squares[7][4].piece != PIECE_ROOK) return false;
    if (gs->board.squares[7][4].color != COLOR_WHITE) return false;
    if (gs->board.squares[6][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// White promotion: ApplyMove replaces pawn with bishop on rank 8.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_White_Bishop(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[6][4] = { PIECE_PAWN, COLOR_WHITE };
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)

    Move pm    = {};
    pm.from_rank  = 6; pm.from_file  = 4;
    pm.to_rank    = 7; pm.to_file    = 4;
    pm.promotion  = PIECE_BISHOP;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    if (gs->board.squares[7][4].piece != PIECE_BISHOP) return false;
    if (gs->board.squares[7][4].color != COLOR_WHITE) return false;
    if (gs->board.squares[6][4].piece != PIECE_NONE)  return false;
    return true;
}

// ---------------------------------------------------------------------------
// White promotion: ApplyMove replaces pawn with knight on rank 8.
// ---------------------------------------------------------------------------
static bool TestPawn_ApplyPromotion_White_Knight(void)
{
    Arena*     arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    GameState* gs = ArenaPushType(arena, GameState);
    InitGameState(gs);

    for (int32 r = 0; r < 8; ++r)
        for (int32 f = 0; f < 8; ++f)
            gs->board.squares[r][f] = { PIECE_NONE, COLOR_NONE };

    gs->board.squares[6][4] = { PIECE_PAWN, COLOR_WHITE };
    gs->board.squares[0][0] = { PIECE_KING, COLOR_WHITE }; // White king on a1 (non-interfering)
    gs->board.squares[7][7] = { PIECE_KING, COLOR_BLACK }; // Black king on h8 (non-interfering)

    Move pm    = {};
    pm.from_rank  = 6; pm.from_file  = 4;
    pm.to_rank    = 7; pm.to_file    = 4;
    pm.promotion  = PIECE_KNIGHT;
    pm.is_en_passant = false;
    pm.is_castling = false;

    ApplyMove(gs, &pm);

    if (gs->board.squares[7][4].piece != PIECE_KNIGHT) return false;
    if (gs->board.squares[7][4].color != COLOR_WHITE) return false;
    if (gs->board.squares[6][4].piece != PIECE_NONE)  return false;
    return true;
}

static const TestEntry k_MovesTests[] = {
    TEST_ENTRY(TestPawn_WhiteDoublePush),
    TEST_ENTRY(TestPawn_BlackDoublePush),
    TEST_ENTRY(TestPawn_BlockedSinglePush),
    TEST_ENTRY(TestPawn_BlockedDoublePush),
    TEST_ENTRY(TestPawn_DiagonalCapture),
    TEST_ENTRY(TestPawn_NoFriendlyCapture),
    TEST_ENTRY(TestPawn_EnPassant),
    TEST_ENTRY(TestPawn_ApplyEnPassant),
    TEST_ENTRY(TestPawn_Promotion),
    TEST_ENTRY(TestPawn_ApplyPromotion),
    TEST_ENTRY(TestPawn_EnPassantTargetTracking),

    TEST_ENTRY(TestKnight_CenterMoves),
    TEST_ENTRY(TestKnight_CornerMoves),
    TEST_ENTRY(TestKnight_NoFriendlyCapture),
    TEST_ENTRY(TestKnight_EnemyCapture),

    TEST_ENTRY(TestRook_OpenBoard),
    TEST_ENTRY(TestRook_BlockedByFriendly),
    TEST_ENTRY(TestRook_CapturesEnemy),

    TEST_ENTRY(TestBishop_OpenBoard),
    TEST_ENTRY(TestBishop_BlockedByFriendly),
    TEST_ENTRY(TestBishop_CapturesEnemy),

    TEST_ENTRY(TestQueen_OpenBoard),
    TEST_ENTRY(TestQueen_BlockedByFriendly),
    TEST_ENTRY(TestQueen_CapturesEnemy),

    TEST_ENTRY(TestIsInCheck_NotInCheck),
    TEST_ENTRY(TestIsInCheck_ByPawn),
    TEST_ENTRY(TestIsInCheck_ByKnight),
    TEST_ENTRY(TestIsInCheck_ByBishop),
    TEST_ENTRY(TestIsInCheck_ByRook),
    TEST_ENTRY(TestIsInCheck_ByQueen),
    TEST_ENTRY(TestIsInCheck_BlockedByPiece),
    TEST_ENTRY(TestIsInCheck_ByKing),

    TEST_ENTRY(TestGetLegalMoves_PinnedRook),
    TEST_ENTRY(TestGetLegalMoves_KingCannotWalkIntoCheck),
    TEST_ENTRY(TestGetLegalMoves_PinnedKnightHasNoMoves),
    TEST_ENTRY(TestGetLegalMoves_PinnedEnemyKnightStillBlocksKingStep),
    TEST_ENTRY(TestGetLegalMoves_CannotCaptureKing),

    TEST_ENTRY(TestCastling_WhiteKingsideAvailable),
    TEST_ENTRY(TestCastling_WhiteQueensideAvailable),
    TEST_ENTRY(TestCastling_KingsideBlockedByPiece),
    TEST_ENTRY(TestCastling_KingsideThroughCheck),
    TEST_ENTRY(TestCastling_ApplyKingside),
    TEST_ENTRY(TestCastling_ApplyQueenside),
    TEST_ENTRY(TestCastling_RightsLostAfterKingMove),
    TEST_ENTRY(TestCastling_RightsLostAfterRookMove),
    TEST_ENTRY(TestCastling_RightsLostAfterRookCaptured),
    TEST_ENTRY(TestCastling_KingsideIllegalWhenPawnAttacksStartAndEnd),
    TEST_ENTRY(TestIsInCheck_PawnDirectlyInFrontIsNotCheck),
    TEST_ENTRY(TestCastling_NormalKingMoveDoesNotMovRook),
    TEST_ENTRY(TestCastling_QueensideIllegalWhenRookAttacksThroughKingSquare),

    TEST_ENTRY(TestEvaluatePosition_Ongoing),
    TEST_ENTRY(TestEvaluatePosition_Checkmate_FoolsMate),
    TEST_ENTRY(TestEvaluatePosition_Stalemate),

    TEST_ENTRY(TestApplyMove_TurnAlternates),
    TEST_ENTRY(TestCastling_ApplyKingside_Black),
    TEST_ENTRY(TestCastling_ApplyQueenside_Black),
    TEST_ENTRY(TestPawn_ApplyEnPassant_Black),
    TEST_ENTRY(TestPawn_ApplyPromotion_Black_Queen),
    TEST_ENTRY(TestPawn_ApplyPromotion_Black_Rook),
    TEST_ENTRY(TestPawn_ApplyPromotion_Black_Bishop),
    TEST_ENTRY(TestPawn_ApplyPromotion_Black_Knight),
    TEST_ENTRY(TestPawn_ApplyPromotion_White_Rook),
    TEST_ENTRY(TestPawn_ApplyPromotion_White_Bishop),
    TEST_ENTRY(TestPawn_ApplyPromotion_White_Knight),
};

void RunMovesTests(AppMemory* memory, int32* passed, int32* total)
{
    ASSERT(memory);
    s_Memory = memory;
    RunTestArray(k_MovesTests, sizeof(k_MovesTests) / sizeof(k_MovesTests[0]),
                 passed, total);
}
