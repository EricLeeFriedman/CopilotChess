#pragma once

#include "types.h"
#include "memory.h"
#include "moves.h"

// All mutable state for mouse-driven piece selection and move input.
// Lives in the input arena; no heap allocation.
struct InputState
{
    int8     selected_rank;  // -1 if nothing is selected
    int8     selected_file;  // -1 if nothing is selected
    bool     has_selection;  // true when a piece is selected
    MoveList legal_moves;    // legal moves for the selected piece (from selected square only)
};

// Initialize input state to "no selection".
void InputInit(InputState* input);

// Map a window pixel coordinate to a board square.
// Returns true and writes *out_rank / *out_file when the pixel is inside the board.
// Returns false when the pixel is outside the board.
bool PixelToSquare(int32 px, int32 py,
                   int32 board_x, int32 board_y, int32 square_size,
                   int8* out_rank, int8* out_file);

// Handle a left mouse button click at window pixel (px, py).
// Updates *input and *gs appropriately:
//   - No selection + current player's piece clicked: select it and cache legal moves.
//   - Selection present + legal destination clicked: apply the move, clear selection.
//   - Selection present + current player's piece clicked: re-select that piece.
//   - Anything else (empty or opponent square): clear selection.
// Returns true if a move was applied to *gs.
bool InputHandleLeftClick(InputState* input, GameState* gs,
                          int32 px, int32 py,
                          int32 board_x, int32 board_y, int32 square_size);

// Clear the current selection (call on right-click or game reset).
void InputClearSelection(InputState* input);
