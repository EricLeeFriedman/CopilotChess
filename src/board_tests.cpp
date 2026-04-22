#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "board.h"
#include "tests.h"

static AppMemory* s_Memory;

static bool TestBoard_WhiteBackRank(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    Board* board = ArenaPushType(arena, Board);
    InitBoard(board);

    const PieceType expected[8] = {
        PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN,
        PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK,
    };

    for (int32 file = 0; file < 8; ++file)
    {
        if (board->squares[0][file].piece != expected[file]) return false;
        if (board->squares[0][file].color != COLOR_WHITE)    return false;
    }
    return true;
}

static bool TestBoard_WhitePawnRank(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    Board* board = ArenaPushType(arena, Board);
    InitBoard(board);

    for (int32 file = 0; file < 8; ++file)
    {
        if (board->squares[1][file].piece != PIECE_PAWN)  return false;
        if (board->squares[1][file].color != COLOR_WHITE) return false;
    }
    return true;
}

static bool TestBoard_EmptyRanks(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    Board* board = ArenaPushType(arena, Board);
    InitBoard(board);

    for (int32 rank = 2; rank <= 5; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            if (board->squares[rank][file].piece != PIECE_NONE)  return false;
            if (board->squares[rank][file].color != COLOR_NONE)  return false;
        }
    }
    return true;
}

static bool TestBoard_BlackPawnRank(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    Board* board = ArenaPushType(arena, Board);
    InitBoard(board);

    for (int32 file = 0; file < 8; ++file)
    {
        if (board->squares[6][file].piece != PIECE_PAWN)  return false;
        if (board->squares[6][file].color != COLOR_BLACK) return false;
    }
    return true;
}

static bool TestBoard_BlackBackRank(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    Board* board = ArenaPushType(arena, Board);
    InitBoard(board);

    const PieceType expected[8] = {
        PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN,
        PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK,
    };

    for (int32 file = 0; file < 8; ++file)
    {
        if (board->squares[7][file].piece != expected[file]) return false;
        if (board->squares[7][file].color != COLOR_BLACK)    return false;
    }
    return true;
}

static bool TestBoard_TotalPieceCount(void)
{
    Arena* arena = &s_Memory->test_scratch;
    ArenaReset(arena);

    Board* board = ArenaPushType(arena, Board);
    InitBoard(board);

    int32 total = 0;
    for (int32 rank = 0; rank < 8; ++rank)
        for (int32 file = 0; file < 8; ++file)
            if (board->squares[rank][file].piece != PIECE_NONE)
                ++total;

    return total == 32;
}

bool RunBoardTests(AppMemory* memory)
{
    s_Memory = memory;

    RUN_TEST(TestBoard_WhiteBackRank);
    RUN_TEST(TestBoard_WhitePawnRank);
    RUN_TEST(TestBoard_EmptyRanks);
    RUN_TEST(TestBoard_BlackPawnRank);
    RUN_TEST(TestBoard_BlackBackRank);
    RUN_TEST(TestBoard_TotalPieceCount);

    return true;
}
