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
    bool      is_castling;  // true = king castles; ApplyMove also repositions the rook
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
    bool  castling_white_kingside;   // White may still castle kingside  (king and h1-rook unmoved)
    bool  castling_white_queenside;  // White may still castle queenside (king and a1-rook unmoved)
    bool  castling_black_kingside;   // Black may still castle kingside  (king and h8-rook unmoved)
    bool  castling_black_queenside;  // Black may still castle queenside (king and a8-rook unmoved)
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
// Steps one square in each of the eight directions. Filters off-board
// squares and squares occupied by friendly pieces.
// Does not clear list->count before appending.
void GenerateKingMoves(const GameState* gs, MoveList* list);

// Append castling moves for gs->side_to_move to 'list'.
// Checks castling rights, piece positions, path clearance, and that
// the king does not start, pass through, or land on an attacked square.
// Does not clear list->count before appending.
void GenerateCastlingMoves(const GameState* gs, MoveList* list);

// Apply a move to the game state: update board, en passant target, and side_to_move.
void ApplyMove(GameState* gs, const Move* move);

// Returns true if the king of 'color' is attacked by any enemy piece on 'board'.
// Uses the existing move generators to enumerate all enemy pseudo-legal attacks.
// Efficient enough to call repeatedly (e.g., during legal move filtering).
bool IsInCheck(const Board* board, Color color);

// Append all fully legal moves for gs->side_to_move to 'out'.
// Generates all pseudo-legal moves (including castling) then discards any
// that leave the moving side's king in check.  Castling is also subject to
// the additional rule that the king may not start or pass through a checked
// square (enforced inside GenerateCastlingMoves before the move is even added
// to the candidate list).
// Does not clear out->count before appending.
void GetLegalMoves(const GameState* gs, MoveList* out);

// Result of evaluating a position.
enum GameResult
{
    GAME_ONGOING,      // At least one legal move remains; game continues.
    GAME_WHITE_WINS,   // Black has no legal moves and is in check (checkmate).
    GAME_BLACK_WINS,   // White has no legal moves and is in check (checkmate).
    GAME_DRAW,         // The side to move has no legal moves and is not in check (stalemate).
};

// Evaluate the position in 'gs' and return the game result.
// Checks whether gs->side_to_move has any legal moves.
//   - No legal moves + king in check  => checkmate (opponent wins).
//   - No legal moves + king not in check => stalemate (draw).
//   - Legal moves available => ongoing.
GameResult EvaluatePosition(const GameState* gs);
