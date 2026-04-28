#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "input.h"

void InputInit(InputState* input)
{
    ASSERT(input);
    input->selected_rank = -1;
    input->selected_file = -1;
    input->has_selection = false;
    input->legal_moves   = {};
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

void InputClearSelection(InputState* input)
{
    ASSERT(input);
    input->selected_rank = -1;
    input->selected_file = -1;
    input->has_selection = false;
    input->legal_moves   = {};
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
            // Prefer queen promotion; return immediately on a hit.
            if (m->promotion == PIECE_QUEEN) return m;
            if (!first_match) first_match = m;
        }
    }
    return first_match;
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

bool InputHandleLeftClick(InputState* input, GameState* gs,
                          int32 px, int32 py,
                          int32 board_x, int32 board_y, int32 square_size)
{
    ASSERT(input && gs);

    int8 rank, file;
    if (!PixelToSquare(px, py, board_x, board_y, square_size, &rank, &file))
    {
        // Click outside the board: clear selection.
        InputClearSelection(input);
        return false;
    }

    if (input->has_selection)
    {
        // Try to find a matching legal move from the selected square to (rank, file).
        const Move* move = FindLegalMove(&input->legal_moves,
                                          input->selected_rank, input->selected_file,
                                          rank, file);
        if (move)
        {
            ApplyMove(gs, move);
            InputClearSelection(input);
            return true;
        }

        // Not a legal target.  Check if it is another piece belonging to
        // the current player — if so, re-select it.
        const Square& sq = gs->board.squares[rank][file];
        if (sq.piece != PIECE_NONE && sq.color == gs->side_to_move)
        {
            input->selected_rank = rank;
            input->selected_file = file;
            input->has_selection = true;
            CacheLegalMovesForSquare(input, gs, rank, file);
            return false;
        }

        // Clicked an empty square or an opponent's piece that is not a legal
        // capture target: clear the selection.
        InputClearSelection(input);
        return false;
    }
    else
    {
        // No current selection.  Select a piece if it belongs to the side to move.
        const Square& sq = gs->board.squares[rank][file];
        if (sq.piece != PIECE_NONE && sq.color == gs->side_to_move)
        {
            input->selected_rank = rank;
            input->selected_file = file;
            input->has_selection = true;
            CacheLegalMovesForSquare(input, gs, rank, file);
        }
        // Clicking an empty square or opponent piece with no selection: no-op.
        return false;
    }
}
