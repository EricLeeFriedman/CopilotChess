#pragma once

#include "types.h"
#include "board.h"

// A single candidate move.
struct Move
{
    int8      from_rank;
    int8      from_file;
    int8      to_rank;
    int8      to_file;
    PieceType promotion;    // PIECE_NONE = normal move; PIECE_QUEEN/ROOK/BISHOP/KNIGHT = promotion piece
    bool      is_en_passant;
    bool      is_castling;  // true when the king castles; ApplyMove moves the rook automatically
};

// Upper bound on candidate moves in any position.
static const int32 MAX_MOVES_PER_POSITION = 256;

// A fixed-size list of candidate moves.
struct MoveList
{
    Move  moves[MAX_MOVES_PER_POSITION];
    int32 count;
};

// All mutable game state needed between moves.
struct GameState
{
    Board board;
    Color side_to_move;
    int8  en_passant_rank; // -1 = no en passant available; 0-7 = rank of the target square
    int8  en_passant_file; // -1 = no en passant available; 0-7 = file of the target square
    bool  castling_wk;     // White may castle kingside
    bool  castling_wq;     // White may castle queenside
    bool  castling_bk;     // Black may castle kingside
    bool  castling_bq;     // Black may castle queenside
};

// Initialize game state to the standard chess starting position.
// White moves first; no en passant available.
void InitGameState(GameState* gs);

// Append all candidate pawn moves for gs->side_to_move to 'list'.
// Does not clear list->count before appending.
void GeneratePawnMoves(const GameState* gs, MoveList* list);

// Append all candidate knight moves for gs->side_to_move to 'list'.
// Knights jump in L-shapes and ignore blocking pieces. Filters off-board
// squares and squares occupied by friendly pieces.
// Does not clear list->count before appending.
void GenerateKnightMoves(const GameState* gs, MoveList* list);

// Append all candidate rook moves for gs->side_to_move to 'list'.
// Casts rays along the four orthogonal directions. Stops after capturing an
// enemy piece (inclusive) or before a friendly piece (exclusive).
// Does not clear list->count before appending.
void GenerateRookMoves(const GameState* gs, MoveList* list);

// Append all candidate bishop moves for gs->side_to_move to 'list'.
// Casts rays along the four diagonal directions. Stops after capturing an
// enemy piece (inclusive) or before a friendly piece (exclusive).
// Does not clear list->count before appending.
void GenerateBishopMoves(const GameState* gs, MoveList* list);

// Append all candidate queen moves for gs->side_to_move to 'list'.
// Combines the rook rays and bishop rays (all eight directions).
// Does not clear list->count before appending.
void GenerateQueenMoves(const GameState* gs, MoveList* list);

// Append all candidate king moves for gs->side_to_move to 'list'.
// Generates moves to all adjacent squares (filtered for board bounds and
// friendly pieces) and castling moves (kingside and queenside) when the
// castling rights allow it, the path is clear, and no attacked square is
// crossed by the king.
// Does not clear list->count before appending.
void GenerateKingMoves(const GameState* gs, MoveList* list);

// Return true if (rank, file) is attacked by any piece of by_color.
// exclude_rank/exclude_file: treat this square as empty during ray-casting
// (pass the king's origin so its presence doesn't shield destination squares
// from sliding-piece attacks along the same ray).
// Pass -1/-1 to disable exclusion.
// Used internally for castling validation and exposed for check detection.
bool IsSquareAttacked(const Board* board, int8 rank, int8 file, Color by_color,
                      int8 exclude_rank = -1, int8 exclude_file = -1);

// Apply a move to the game state: update board, en passant target, and side_to_move.
void ApplyMove(GameState* gs, const Move* move);
