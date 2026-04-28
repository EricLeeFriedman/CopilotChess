#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "input.h"

void InputInit(InputState* input)
{
    ASSERT(input);
    input->dragging       = false;
    input->drag_from_rank = -1;
    input->drag_from_file = -1;
    input->drag_cursor_x  = 0;
    input->drag_cursor_y  = 0;
    input->legal_moves    = {};
}

bool PixelToSquare(int32 px, int32 py,
                   int32 board_x, int32 board_y, int32 square_size,
                   int8* out_rank, int8* out_file)
{
    ASSERT(out_rank && out_file);
    ASSERT(square_size > 0);

    int32 local_x = px - board_x;
    int32 local_y = py - board_y;
    int32 board_px = square_size * 8;

    if (local_x < 0 || local_x >= board_px ||
        local_y < 0 || local_y >= board_px)
    {
        return false;
    }

    // Rank 0 (White's back rank) is rendered at the bottom of the board;
    // rank 7 (Black's back rank) is at the top.  Convert from top-down pixels.
    *out_file = (int8)(local_x / square_size);
    *out_rank = (int8)(7 - (local_y / square_size));
    return true;
}

void InputCancelDrag(InputState* input)
{
    ASSERT(input);
    input->dragging       = false;
    input->drag_from_rank = -1;
    input->drag_from_file = -1;
    input->drag_cursor_x  = 0;
    input->drag_cursor_y  = 0;
    input->legal_moves    = {};
}

// Populate input->legal_moves with only the moves that originate from
// (rank, file) in the full legal move list for gs->side_to_move.
static void CacheLegalMovesForSquare(InputState* input, const GameState* gs,
                                      int8 rank, int8 file)
{
    MoveList all = {};
    GetLegalMoves(gs, &all);

    input->legal_moves = {};
    for (int32 i = 0; i < all.count; ++i)
    {
        const Move& m = all.moves[i];
        if (m.from_rank == rank && m.from_file == file)
        {
            input->legal_moves.moves[input->legal_moves.count++] = m;
        }
    }
}

void InputHandleDragStart(InputState* input, const GameState* gs,
                          int32 px, int32 py,
                          int32 board_x, int32 board_y, int32 square_size)
{
    ASSERT(input && gs);

    // Cancel any prior drag (e.g. if button-up was missed).
    InputCancelDrag(input);

    int8 rank, file;
    if (!PixelToSquare(px, py, board_x, board_y, square_size, &rank, &file))
        return;

    const Square& sq = gs->board.squares[rank][file];
    if (sq.piece == PIECE_NONE || sq.color != gs->side_to_move)
        return;

    input->dragging       = true;
    input->drag_from_rank = rank;
    input->drag_from_file = file;
    input->drag_cursor_x  = px;
    input->drag_cursor_y  = py;
    CacheLegalMovesForSquare(input, gs, rank, file);
}

void InputHandleDragMove(InputState* input, int32 px, int32 py)
{
    ASSERT(input);
    if (!input->dragging) return;
    input->drag_cursor_x = px;
    input->drag_cursor_y = py;
}

// Find the best matching legal move for the given from/to pair.
// When a promotion is involved, prefers PIECE_QUEEN (auto-queen).
// Returns nullptr if no matching move exists.
static const Move* FindLegalMove(const MoveList* legal_moves,
                                  int8 from_rank, int8 from_file,
                                  int8 to_rank,   int8 to_file)
{
    const Move* first_match = nullptr;
    for (int32 i = 0; i < legal_moves->count; ++i)
    {
        const Move* m = &legal_moves->moves[i];
        if (m->from_rank == from_rank && m->from_file == from_file &&
            m->to_rank   == to_rank   && m->to_file   == to_file)
        {
            if (m->promotion == PIECE_QUEEN) return m;
            if (!first_match) first_match = m;
        }
    }
    return first_match;
}

bool InputHandleDragEnd(InputState* input, GameState* gs,
                        int32 px, int32 py,
                        int32 board_x, int32 board_y, int32 square_size)
{
    ASSERT(input && gs);

    if (!input->dragging)
        return false;

    int8 to_rank, to_file;
    bool on_board = PixelToSquare(px, py, board_x, board_y, square_size,
                                   &to_rank, &to_file);

    if (on_board)
    {
        const Move* move = FindLegalMove(&input->legal_moves,
                                          input->drag_from_rank, input->drag_from_file,
                                          to_rank, to_file);
        if (move)
        {
            ApplyMove(gs, move);
            InputCancelDrag(input);
            return true;
        }
    }

    // Illegal target, outside board, or dropped on source square — cancel drag.
    InputCancelDrag(input);
    return false;
}
