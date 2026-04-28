#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "input.h"

// Promotion piece order — shared between InputHandlePromotionClick and
// DrawPromotionPicker (ui.cpp uses the same order independently).
static const PieceType k_PromoPieces[4] = {
    PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT
};

void InputInit(InputState* input)
{
    ASSERT(input);
    input->dragging          = false;
    input->drag_from_rank    = -1;
    input->drag_from_file    = -1;
    input->drag_cursor_x     = 0;
    input->drag_cursor_y     = 0;
    input->legal_moves       = {};
    input->pending_promotion = false;
    input->promo_from_rank   = -1;
    input->promo_from_file   = -1;
    input->promo_to_rank     = -1;
    input->promo_to_file     = -1;
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
    input->dragging          = false;
    input->drag_from_rank    = -1;
    input->drag_from_file    = -1;
    input->drag_cursor_x     = 0;
    input->drag_cursor_y     = 0;
    input->legal_moves       = {};
    input->pending_promotion = false;
    input->promo_from_rank   = -1;
    input->promo_from_file   = -1;
    input->promo_to_rank     = -1;
    input->promo_to_file     = -1;
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

// Returns true if any legal move from (from_rank, from_file) to (to_rank,
// to_file) is a promotion move.  Used to decide whether to enter picker mode.
static bool IsPromotionTarget(const MoveList* legal_moves,
                               int8 from_rank, int8 from_file,
                               int8 to_rank,   int8 to_file)
{
    for (int32 i = 0; i < legal_moves->count; ++i)
    {
        const Move& m = legal_moves->moves[i];
        if (m.from_rank == from_rank && m.from_file == from_file &&
            m.to_rank   == to_rank   && m.to_file   == to_file   &&
            m.promotion != PIECE_NONE)
            return true;
    }
    return false;
}

// Find the first non-promotion legal move matching from/to.  Promotion moves
// are handled exclusively through InputHandlePromotionClick.
static const Move* FindLegalMove(const MoveList* legal_moves,
                                  int8 from_rank, int8 from_file,
                                  int8 to_rank,   int8 to_file)
{
    for (int32 i = 0; i < legal_moves->count; ++i)
    {
        const Move* m = &legal_moves->moves[i];
        if (m->from_rank == from_rank && m->from_file == from_file &&
            m->to_rank   == to_rank   && m->to_file   == to_file   &&
            m->promotion == PIECE_NONE)
            return m;
    }
    return nullptr;
}

bool InputHandleDragEnd(InputState* input, GameState* gs,
                        int32 px, int32 py,
                        int32 board_x, int32 board_y, int32 square_size)
{
    ASSERT(input && gs);

    if (!input->dragging)
        return false;

    int8 from_rank = input->drag_from_rank;
    int8 from_file = input->drag_from_file;

    int8 to_rank, to_file;
    bool on_board = PixelToSquare(px, py, board_x, board_y, square_size,
                                   &to_rank, &to_file);

    if (on_board)
    {
        // Check for pawn promotion first — enter picker mode instead of
        // applying a single auto-chosen piece.
        if (IsPromotionTarget(&input->legal_moves,
                              from_rank, from_file, to_rank, to_file))
        {
            // Partial clear: end drag but keep legal_moves alive for the picker.
            input->dragging       = false;
            input->drag_from_rank = -1;
            input->drag_from_file = -1;
            input->drag_cursor_x  = 0;
            input->drag_cursor_y  = 0;

            input->pending_promotion = true;
            input->promo_from_rank   = from_rank;
            input->promo_from_file   = from_file;
            input->promo_to_rank     = to_rank;
            input->promo_to_file     = to_file;
            return false; // move applied later by InputHandlePromotionClick
        }

        const Move* move = FindLegalMove(&input->legal_moves,
                                          from_rank, from_file,
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

// Returns the picker index (0 = Queen, 1 = Rook, 2 = Bishop, 3 = Knight) for
// a given board square during a pending promotion, or -1 if the square is not
// part of the picker.
static int32 GetPickerIndex(const InputState* input, Color promoting_side,
                             int8 rank, int8 file)
{
    if (file != input->promo_to_file) return -1;

    if (promoting_side == COLOR_WHITE)
    {
        // White: picker descends from rank 7 (index 0) to rank 4 (index 3).
        int32 idx = 7 - (int32)rank;
        if (idx >= 0 && idx < 4) return idx;
    }
    else
    {
        // Black: picker ascends from rank 0 (index 0) to rank 3 (index 3).
        int32 idx = (int32)rank;
        if (idx >= 0 && idx < 4) return idx;
    }
    return -1;
}

bool InputHandlePromotionClick(InputState* input, GameState* gs,
                               int32 px, int32 py,
                               int32 board_x, int32 board_y, int32 square_size)
{
    ASSERT(input && gs);

    if (!input->pending_promotion)
        return false;

    int8 rank, file;
    if (!PixelToSquare(px, py, board_x, board_y, square_size, &rank, &file))
    {
        // Click outside the board — cancel pending promotion.
        InputCancelDrag(input);
        return false;
    }

    int32 idx = GetPickerIndex(input, gs->side_to_move, rank, file);
    if (idx < 0)
    {
        // Click outside the promotion picker — cancel.
        InputCancelDrag(input);
        return false;
    }

    PieceType promo = k_PromoPieces[idx];

    // Find and apply the matching promotion move.
    for (int32 i = 0; i < input->legal_moves.count; ++i)
    {
        const Move& m = input->legal_moves.moves[i];
        if (m.from_rank == input->promo_from_rank &&
            m.from_file == input->promo_from_file &&
            m.to_rank   == input->promo_to_rank   &&
            m.to_file   == input->promo_to_file   &&
            m.promotion == promo)
        {
            ApplyMove(gs, &m);
            InputCancelDrag(input);
            return true;
        }
    }

    // Should be unreachable if legal_moves was built correctly.
    InputCancelDrag(input);
    return false;
}
