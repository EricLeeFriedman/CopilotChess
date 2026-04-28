#pragma once

#include "renderer.h"
#include "moves.h"

// Draw the complete board view (squares, highlights, and pieces) into the
// renderer's pixel buffer.
//
//   board_x / board_y  – top-left pixel position of the board
//   square_size        – pixel size of each square (board spans 8 * square_size)
//   selected_rank/file – currently selected/drag-source square; pass -1 for both if nothing selected
//   legal_moves        – legal moves for the selected piece; may be null or count == 0
//   hide_rank/file     – when both are >= 0, the piece on that square is not drawn (used during
//                        drag so the piece appears only at the cursor, not also on the board)
void DrawBoard(RendererState*   rs,
               const GameState* gs,
               int32            board_x,
               int32            board_y,
               int32            square_size,
               int8             selected_rank,
               int8             selected_file,
               const MoveList*  legal_moves,
               int8             hide_rank  = -1,
               int8             hide_file  = -1);

// Draw a single piece centered at pixel (center_x, center_y).
// Used to render the floating piece under the cursor during drag.
void DrawPieceAt(RendererState* rs,
                 PieceType      type,
                 Color          piece_color,
                 int32          center_x,
                 int32          center_y,
                 int32          sq_size);
