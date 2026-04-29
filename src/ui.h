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

// Draw the promotion piece picker over the board.
// Renders four highlighted squares (Queen, Rook, Bishop, Knight in that order)
// at the promoting side's target file, starting from the promotion rank and
// descending (White) or ascending (Black) toward the board centre.
// Call after DrawBoard when InputState::pending_promotion is true.
void DrawPromotionPicker(RendererState*   rs,
                         int32            board_x,
                         int32            board_y,
                         int32            square_size,
                         int8             to_rank,
                         int8             to_file,
                         Color            promoting_side);

// Draw the game-over overlay (result indicator + restart button) on top of
// the board.  Call after DrawBoard when EvaluatePosition returns a non-ONGOING
// result.  result must be GAME_WHITE_WINS, GAME_BLACK_WINS, or GAME_DRAW.
void DrawGameOverOverlay(RendererState*   rs,
                         GameResult       result,
                         int32            board_x,
                         int32            board_y,
                         int32            square_size);

// Returns true when (px, py) falls within the restart button drawn by
// DrawGameOverOverlay.  Pass the same board_x/board_y/square_size used in the
// draw call.
bool IsRestartButtonHit(int32 px, int32 py,
                        int32 board_x, int32 board_y, int32 square_size);
