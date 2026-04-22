#pragma once

#include "types.h"
#include "memory.h"

// Piece types. PIECE_NONE means the square is empty.
enum PieceType : uint8
{
    PIECE_NONE   = 0,
    PIECE_PAWN   = 1,
    PIECE_KNIGHT = 2,
    PIECE_BISHOP = 3,
    PIECE_ROOK   = 4,
    PIECE_QUEEN  = 5,
    PIECE_KING   = 6,
};

// Piece color. COLOR_NONE is used for empty squares.
enum Color : uint8
{
    COLOR_NONE  = 0,
    COLOR_WHITE = 1,
    COLOR_BLACK = 2,
};

// A single square on the board.
struct Square
{
    PieceType piece;
    Color     color;
};

// The 8x8 board. squares[rank][file] where rank 0 is White's back rank (rank 1)
// and rank 7 is Black's back rank (rank 8). Files 0–7 map to a–h.
struct Board
{
    Square squares[8][8];
};

// Initialise board to the standard chess starting position.
// The caller must supply a valid Board pointer (e.g. pushed from the game_state arena).
void InitBoard(Board* board);
