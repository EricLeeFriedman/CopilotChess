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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

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

    MoveList list = {};
    GeneratePawnMoves(gs, COLOR_BLACK, &list);

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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

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
// Promotion: pawn on the 7th rank (rank 6) promotes to queen.
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
    GeneratePawnMoves(gs, COLOR_WHITE, &list);

    // Move to rank 7 (e8) must be a queen promotion.
    if (!FindPromotion(&list, 6, 4, 7, 4, PIECE_QUEEN)) return false;
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

    return true;
}
