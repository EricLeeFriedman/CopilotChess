#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "board.h"

static const PieceType BACK_RANK[8] =
{
    PIECE_ROOK,
    PIECE_KNIGHT,
    PIECE_BISHOP,
    PIECE_QUEEN,
    PIECE_KING,
    PIECE_BISHOP,
    PIECE_KNIGHT,
    PIECE_ROOK,
};

void InitBoard(Board* board)
{
    ASSERT(board);

    // Clear every square first.
    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            board->squares[rank][file] = { PIECE_NONE, COLOR_NONE };
        }
    }

    // White back rank (rank index 0) and pawns (rank index 1).
    for (int32 file = 0; file < 8; ++file)
    {
        board->squares[0][file] = { BACK_RANK[file], COLOR_WHITE };
        board->squares[1][file] = { PIECE_PAWN,      COLOR_WHITE };
    }

    // Black pawns (rank index 6) and back rank (rank index 7).
    for (int32 file = 0; file < 8; ++file)
    {
        board->squares[6][file] = { PIECE_PAWN,      COLOR_BLACK };
        board->squares[7][file] = { BACK_RANK[file], COLOR_BLACK };
    }
}
