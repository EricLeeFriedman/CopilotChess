#pragma once

#include "types.h"
#include "memory.h"
#include "moves.h"

// All mutable state for mouse-driven drag-and-drop piece movement.
// Lives in the input arena; no heap allocation.
struct InputState
{
    // --- drag state (active while LMB is held and a piece is being dragged) ---
    bool     dragging;        // true while left button is held and a piece is being dragged
    int8     drag_from_rank;  // source square rank (-1 when not dragging)
    int8     drag_from_file;  // source square file (-1 when not dragging)
    int32    drag_cursor_x;   // current cursor pixel x (updated every WM_MOUSEMOVE)
    int32    drag_cursor_y;   // current cursor pixel y
    MoveList legal_moves;     // legal moves for the dragged/promoting piece (from source square)

    // --- promotion picker state (active after a pawn is dropped on the last rank) ---
    bool     pending_promotion; // true when player must choose a promotion piece
    int8     promo_from_rank;   // source square of the promoting pawn
    int8     promo_from_file;
    int8     promo_to_rank;     // target square on the promotion rank
    int8     promo_to_file;
};

// Initialize input state to "not dragging, no pending promotion".
void InputInit(InputState* input);

// Map a window pixel coordinate to a board square.
// Returns true and writes *out_rank / *out_file when the pixel is inside the board.
// Returns false when the pixel is outside the board.
bool PixelToSquare(int32 px, int32 py,
                   int32 board_x, int32 board_y, int32 square_size,
                   int8* out_rank, int8* out_file);

// Handle WM_LBUTTONDOWN: begin dragging the piece at (px, py) if it belongs
// to the current player.  Does nothing when the square is empty or belongs to
// the opponent.
void InputHandleDragStart(InputState* input, const GameState* gs,
                          int32 px, int32 py,
                          int32 board_x, int32 board_y, int32 square_size);

// Handle WM_MOUSEMOVE: update the cursor position during an active drag.
// Safe to call when not dragging (no-op).
void InputHandleDragMove(InputState* input, int32 px, int32 py);

// Handle WM_LBUTTONUP: attempt to drop the dragged piece onto the square under
// the cursor.
//   - If the drop targets a non-promotion legal square, applies the move and
//     returns true.
//   - If the drop is a pawn promotion, sets pending_promotion and returns false
//     (the caller must then call InputHandlePromotionClick on the next click).
//   - In all other cases (illegal target, outside board, same source square)
//     cancels the drag and returns false.
// Safe to call when not dragging (no-op returning false).
bool InputHandleDragEnd(InputState* input, GameState* gs,
                        int32 px, int32 py,
                        int32 board_x, int32 board_y, int32 square_size);

// Handle WM_LBUTTONDOWN when pending_promotion is true.
// Maps the click to one of the four promotion-piece picker squares shown on
// the board (Queen, Rook, Bishop, Knight, in order from the promotion rank
// toward the board center for the promoting side).
//   - If the click hits a valid picker square, applies the chosen promotion
//     move to *gs and returns true.
//   - If the click is outside the picker (or outside the board), cancels the
//     pending promotion without changing *gs and returns false.
bool InputHandlePromotionClick(InputState* input, GameState* gs,
                               int32 px, int32 py,
                               int32 board_x, int32 board_y, int32 square_size);

// Cancel any active drag or pending promotion (call on WM_RBUTTONDOWN or
// focus loss).
void InputCancelDrag(InputState* input);
