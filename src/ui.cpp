#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui.h"

// ---------------------------------------------------------------------------
// Colour palette (Pixel layout: { b, g, r, a })
// ---------------------------------------------------------------------------

// Board square colours — classic chess.com palette
static const Pixel BOARD_LIGHT    = { 181, 217, 240, 0 };  // #F0D9B5 light square
static const Pixel BOARD_DARK     = {  99, 136, 181, 0 };  // #B58863 dark square
static const Pixel BOARD_SELECTED = { 105, 151, 130, 0 };  // green tint – selected square
static const Pixel BOARD_MOVE_DOT = {  64, 111, 100, 0 };  // darker green – valid-move indicator

// Piece body colours
static const Pixel PIECE_W_FILL   = { 230, 245, 245, 0 };  // near-white piece fill
static const Pixel PIECE_W_BORDER = {  60,  60,  60, 0 };  // dark outline for white pieces
static const Pixel PIECE_B_FILL   = {  40,  40,  40, 0 };  // near-black piece fill
static const Pixel PIECE_B_BORDER = { 200, 200, 200, 0 };  // light outline for black pieces

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Returns true if the square (rank, file) is a target in legal_moves.
static bool IsLegalTarget(const MoveList* legal_moves, int8 rank, int8 file)
{
    if (!legal_moves) return false;
    for (int32 i = 0; i < legal_moves->count; ++i)
    {
        if (legal_moves->moves[i].to_rank == rank &&
            legal_moves->moves[i].to_file == file)
            return true;
    }
    return false;
}

// Draw a single chess piece at the given square top-left (sq_x, sq_y).
// All dimensions are proportional to sq_size (designed at sq_size = 80).
static void DrawPiece(RendererState* rs,
                      int32 sq_x, int32 sq_y, int32 sq_size,
                      PieceType type, Color piece_color)
{
    int32 cx = sq_x + sq_size / 2;

    // base_y: pixel just above the bottom margin of the square
    int32 base_y = sq_y + sq_size - sq_size * 10 / 100;

    Pixel fill   = (piece_color == COLOR_WHITE) ? PIECE_W_FILL   : PIECE_B_FILL;
    Pixel border = (piece_color == COLOR_WHITE) ? PIECE_W_BORDER : PIECE_B_BORDER;

    // All dimensions are scaled from the sq_size=80 reference.
    // Integer division truncates; that is intentional to keep shapes crisp.
#define S(n) ((n) * sq_size / 80)

    switch (type)
    {
        // ----------------------------------------------------------------
        // PAWN – circle (head) + flat base
        // ----------------------------------------------------------------
        case PIECE_PAWN:
        {
            int32 r   = S(14);
            int32 hcy = base_y - S(22);       // head centre

            // head
            DrawFilledCircle(rs, cx, hcy, r + 2, border);
            DrawFilledCircle(rs, cx, hcy, r,     fill);

            // base
            int32 bw = S(28), bh = S(8);
            int32 bx = cx - bw / 2;
            int32 by = base_y - bh;
            DrawRect(rs, bx - 1, by - 1, bw + 2, bh + 2, border);
            DrawRect(rs, bx,     by,     bw,     bh,     fill);
        } break;

        // ----------------------------------------------------------------
        // ROOK – rectangular tower with three battlements
        // ----------------------------------------------------------------
        case PIECE_ROOK:
        {
            int32 tw = S(32), th = S(36);
            int32 tx = cx - tw / 2;
            int32 ty = base_y - th;

            // main tower
            DrawRect(rs, tx - 1, ty - 1, tw + 2, th + 2, border);
            DrawRect(rs, tx,     ty,     tw,     th,     fill);

            // 3 merlons on top; each 8 px wide with 4 px gaps, totalling 32 px
            int32 mw = S(8), mh = S(10);
            int32 my = ty - mh;
            int32 merlon_x[3] = { tx, tx + S(12), tx + S(24) };
            for (int32 m = 0; m < 3; ++m)
            {
                DrawRect(rs, merlon_x[m] - 1, my - 1, mw + 2, mh + 2, border);
                DrawRect(rs, merlon_x[m],     my,     mw,     mh,     fill);
            }
        } break;

        // ----------------------------------------------------------------
        // KNIGHT – asymmetric staircase silhouette (horse's head and neck)
        // ----------------------------------------------------------------
        case PIECE_KNIGHT:
        {
            // base
            int32 bw = S(28), bh = S(8);
            int32 bx = cx - bw / 2;
            int32 by = base_y - bh;
            DrawRect(rs, bx - 1, by - 1, bw + 2, bh + 2, border);
            DrawRect(rs, bx,     by,     bw,     bh,     fill);

            // body / chest (lower block, slightly left-shifted)
            int32 body_w = S(24), body_h = S(20);
            int32 body_x = cx - body_w / 2 - S(2);
            int32 body_y = by - body_h;
            DrawRect(rs, body_x - 1, body_y - 1, body_w + 2, body_h + 2, border);
            DrawRect(rs, body_x,     body_y,     body_w,     body_h,     fill);

            // head (upper block, right-shifted relative to body)
            int32 head_w = S(18), head_h = S(18);
            int32 head_x = cx;
            int32 head_y = body_y - head_h;
            DrawRect(rs, head_x - 1, head_y - 1, head_w + 2, head_h + 2, border);
            DrawRect(rs, head_x,     head_y,     head_w,     head_h,     fill);

            // snout (extends left from head centre)
            int32 sn_w = S(14), sn_h = S(8);
            int32 sn_x = cx - S(12);
            int32 sn_y = head_y + head_h / 2 - sn_h / 2;
            DrawRect(rs, sn_x - 1, sn_y - 1, sn_w + 2, sn_h + 2, border);
            DrawRect(rs, sn_x,     sn_y,     sn_w,     sn_h,     fill);
        } break;

        // ----------------------------------------------------------------
        // BISHOP – ball on a narrow stem above a flat base + tiny tip
        // ----------------------------------------------------------------
        case PIECE_BISHOP:
        {
            // base
            int32 bw = S(30), bh = S(8);
            int32 bx = cx - bw / 2;
            int32 by = base_y - bh;
            DrawRect(rs, bx - 1, by - 1, bw + 2, bh + 2, border);
            DrawRect(rs, bx,     by,     bw,     bh,     fill);

            // stem
            int32 sw = S(10), sh = S(22);
            int32 sx = cx - sw / 2;
            int32 sy = by - sh;
            DrawRect(rs, sx - 1, sy - 1, sw + 2, sh + 2, border);
            DrawRect(rs, sx,     sy,     sw,     sh,     fill);

            // ball
            int32 r   = S(11);
            int32 bcy = sy - r;
            DrawFilledCircle(rs, cx, bcy, r + 2, border);
            DrawFilledCircle(rs, cx, bcy, r,     fill);

            // tip (small orb on top of ball)
            int32 tr  = S(3);
            int32 tcy = bcy - r - tr;
            DrawFilledCircle(rs, cx, tcy, tr + 1, border);
            DrawFilledCircle(rs, cx, tcy, tr,     fill);
        } break;

        // ----------------------------------------------------------------
        // QUEEN – large circle body with three crown orbs
        // ----------------------------------------------------------------
        case PIECE_QUEEN:
        {
            // body circle
            int32 r   = S(17);
            int32 bcy = base_y - r - S(2);
            DrawFilledCircle(rs, cx, bcy, r + 2, border);
            DrawFilledCircle(rs, cx, bcy, r,     fill);

            // three crown orbs above the body
            int32 orb_r = S(5);
            int32 orb_y = bcy - r - orb_r;

            // centre orb (slightly higher)
            DrawFilledCircle(rs, cx,        orb_y - S(2), orb_r + 1, border);
            DrawFilledCircle(rs, cx,        orb_y - S(2), orb_r,     fill);
            // left orb
            DrawFilledCircle(rs, cx - S(12), orb_y,       orb_r + 1, border);
            DrawFilledCircle(rs, cx - S(12), orb_y,       orb_r,     fill);
            // right orb
            DrawFilledCircle(rs, cx + S(12), orb_y,       orb_r + 1, border);
            DrawFilledCircle(rs, cx + S(12), orb_y,       orb_r,     fill);
        } break;

        // ----------------------------------------------------------------
        // KING – rectangular body with a prominent cross on top
        // ----------------------------------------------------------------
        case PIECE_KING:
        {
            // body
            int32 bw = S(24), bh = S(24);
            int32 bx = cx - bw / 2;
            int32 by = base_y - bh;
            DrawRect(rs, bx - 1, by - 1, bw + 2, bh + 2, border);
            DrawRect(rs, bx,     by,     bw,     bh,     fill);

            // cross – vertical arm
            int32 vw = S(8), vh = S(18);
            int32 vx = cx - vw / 2;
            int32 vy = by - vh;
            DrawRect(rs, vx - 1, vy - 1, vw + 2, vh + 2, border);
            DrawRect(rs, vx,     vy,     vw,     vh,     fill);

            // cross – horizontal arm (centred on the vertical arm)
            int32 hw = S(24), hh = S(8);
            int32 hx = cx - hw / 2;
            int32 hy = vy + vh / 2 - hh / 2;
            DrawRect(rs, hx - 1, hy - 1, hw + 2, hh + 2, border);
            DrawRect(rs, hx,     hy,     hw,     hh,     fill);
        } break;

        default: break;
    }

#undef S
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void DrawBoard(RendererState*   rs,
               const GameState* gs,
               int32            board_x,
               int32            board_y,
               int32            square_size,
               int8             selected_rank,
               int8             selected_file,
               const MoveList*  legal_moves,
               int8             hide_rank,
               int8             hide_file)
{
    ASSERT(rs && gs);

    // -----------------------------------------------------------------------
    // Pass 1: draw the 64 squares
    // -----------------------------------------------------------------------
    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            int32 sq_x = board_x + file * square_size;
            // Rank 0 is White's back rank; render it at the bottom of the board
            // so rank 7 (Black's back rank) appears at the top.
            int32 sq_y = board_y + (7 - rank) * square_size;

            // Base square colour: light on (rank+file) odd, dark on even
            // (a1 = rank 0, file 0, sum 0 → dark; h1 = rank 0, file 7, sum 7 → light)
            bool  is_light = ((rank + file) & 1) != 0;
            Pixel sq_color = is_light ? BOARD_LIGHT : BOARD_DARK;

            // Overlay: selected square or valid-move target
            if (rank == selected_rank && file == selected_file)
            {
                sq_color = BOARD_SELECTED;
            }
            else if (IsLegalTarget(legal_moves, (int8)rank, (int8)file))
            {
                sq_color = BOARD_SELECTED;
            }

            DrawRect(rs, sq_x, sq_y, square_size, square_size, sq_color);

            // Valid-move dot: drawn on top of the tinted square for empty targets
            if (IsLegalTarget(legal_moves, (int8)rank, (int8)file))
            {
                const Square& sq = gs->board.squares[rank][file];
                int32 cx  = sq_x + square_size / 2;
                int32 cy  = sq_y + square_size / 2;
                int32 dot_r = square_size * 14 / 100;
                if (sq.piece == PIECE_NONE)
                {
                    DrawFilledCircle(rs, cx, cy, dot_r, BOARD_MOVE_DOT);
                }
                else
                {
                    // Enemy piece on the target: draw a ring around the square edge
                    int32 ring_w = square_size * 6 / 100;
                    DrawRect(rs, sq_x,                       sq_y,                       square_size, ring_w, BOARD_MOVE_DOT);
                    DrawRect(rs, sq_x,                       sq_y + square_size - ring_w, square_size, ring_w, BOARD_MOVE_DOT);
                    DrawRect(rs, sq_x,                       sq_y,                       ring_w, square_size, BOARD_MOVE_DOT);
                    DrawRect(rs, sq_x + square_size - ring_w, sq_y,                       ring_w, square_size, BOARD_MOVE_DOT);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Pass 2: draw the pieces
    // -----------------------------------------------------------------------
    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            // Skip the square whose piece is being dragged — it will be drawn
            // at the cursor position by the caller instead.
            if (rank == hide_rank && file == hide_file) continue;

            const Square& sq = gs->board.squares[rank][file];
            if (sq.piece == PIECE_NONE) continue;

            int32 sq_x = board_x + file * square_size;
            int32 sq_y = board_y + (7 - rank) * square_size;

            DrawPiece(rs, sq_x, sq_y, square_size, sq.piece, sq.color);
        }
    }
}

// ---------------------------------------------------------------------------
// Public helper — floating piece during drag
// ---------------------------------------------------------------------------

void DrawPieceAt(RendererState* rs,
                 PieceType      type,
                 Color          piece_color,
                 int32          center_x,
                 int32          center_y,
                 int32          sq_size)
{
    // DrawPiece expects the top-left corner of the square, so back-compute
    // it from the desired center.
    int32 sq_x = center_x - sq_size / 2;
    int32 sq_y = center_y - sq_size / 2;
    DrawPiece(rs, sq_x, sq_y, sq_size, type, piece_color);
}
