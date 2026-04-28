#pragma once

#include "renderer.h"
#include "moves.h"

// Draw the complete board view (squares, highlights, and pieces) into the
// renderer's pixel buffer.
//
//   board_x / board_y  – top-left pixel position of the board
//   square_size        – pixel size of each square (board spans 8 * square_size)
//   selected_rank/file – currently selected square; pass -1 for both if nothing is selected
//   legal_moves        – legal moves for the selected piece; may be null or count == 0
void DrawBoard(RendererState*   rs,
               const GameState* gs,
               int32            board_x,
               int32            board_y,
               int32            square_size,
               int8             selected_rank,
               int8             selected_file,
               const MoveList*  legal_moves);
