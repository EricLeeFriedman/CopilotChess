#pragma once

#include "types.h"
#include "memory.h"
#include "moves.h"

// All mutable state for mouse-driven drag-and-drop piece movement.
// Lives in the input arena; no heap allocation.
struct InputState
{
    bool     dragging;        // true while left button is held and a piece is being dragged
    int8     drag_from_rank;  // source square rank (-1 when not dragging)
    int8     drag_from_file;  // source square file (-1 when not dragging)
    int32    drag_cursor_x;   // current cursor pixel x (updated every WM_MOUSEMOVE)
    int32    drag_cursor_y;   // current cursor pixel y
    MoveList legal_moves;     // legal moves for the dragged piece; pre-computed at drag start
};

// Initialize input state to "not dragging".
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
// the cursor.  If the drop is on a legal destination, applies the move to *gs
// and returns true.  In all other cases (illegal target, outside board, or same
// source square) the drag is cancelled without changing *gs and returns false.
// Safe to call when not dragging (no-op returning false).
bool InputHandleDragEnd(InputState* input, GameState* gs,
                        int32 px, int32 py,
                        int32 board_x, int32 board_y, int32 square_size);

// Cancel any active drag without applying a move (call on WM_RBUTTONDOWN or
// on focus loss).
void InputCancelDrag(InputState* input);
