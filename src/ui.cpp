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
static const Pixel BOARD_CHECK    = {  50,  50, 200, 0 };  // red tint – king square when in check

// Status overlay colours
static const Pixel STATUS_BANNER_BG  = {  20,  20,  20, 0 };  // very dark banner background
static const Pixel STATUS_TEXT_COLOR = { 220, 220, 220, 0 };  // near-white message text

// Piece body colours
static const Pixel PIECE_W_FILL   = { 230, 245, 245, 0 };  // near-white piece fill
static const Pixel PIECE_W_BORDER = {  60,  60,  60, 0 };  // dark outline for white pieces
static const Pixel PIECE_B_FILL   = {  40,  40,  40, 0 };  // near-black piece fill
static const Pixel PIECE_B_BORDER = { 200, 200, 200, 0 };  // light outline for black pieces

// ---------------------------------------------------------------------------
// Minimal 5×7 pixel font for in-game status messages.
// Each glyph is 7 bytes; each byte is a 5-bit row mask (bit 4 = leftmost pixel).
// Only the characters needed for status strings are defined.
// ---------------------------------------------------------------------------

struct Glyph5x7
{
    uint8 rows[7];
};

// Returns a pointer to the 5×7 glyph for the given ASCII character.
// Returns a pointer to a blank glyph for characters without a definition.
static const Glyph5x7* GetGlyph(uint8 ch)
{
    static const Glyph5x7 k_Blank = {{ 0, 0, 0, 0, 0, 0, 0 }};

    static const Glyph5x7 k_Space  = {{ 0,  0,  0,  0,  0,  0,  0  }};
    static const Glyph5x7 k_Hyphen = {{ 0,  0,  0,  31, 0,  0,  0  }};
    static const Glyph5x7 k_A = {{ 14, 17, 17, 31, 17, 17, 17 }};
    static const Glyph5x7 k_B = {{ 30, 17, 17, 30, 17, 17, 30 }};
    static const Glyph5x7 k_C = {{ 14, 16, 16, 16, 16, 16, 14 }};
    static const Glyph5x7 k_D = {{ 30, 17, 17, 17, 17, 17, 30 }};
    static const Glyph5x7 k_E = {{ 31, 16, 16, 30, 16, 16, 31 }};
    static const Glyph5x7 k_H = {{ 17, 17, 17, 31, 17, 17, 17 }};
    static const Glyph5x7 k_I = {{ 31,  4,  4,  4,  4,  4, 31 }};
    static const Glyph5x7 k_K = {{ 17, 18, 20, 24, 20, 18, 17 }};
    static const Glyph5x7 k_L = {{ 16, 16, 16, 16, 16, 16, 31 }};
    static const Glyph5x7 k_M = {{ 17, 27, 21, 17, 17, 17, 17 }};
    static const Glyph5x7 k_N = {{ 17, 25, 21, 19, 17, 17, 17 }};
    static const Glyph5x7 k_R = {{ 30, 17, 17, 30, 20, 18, 17 }};
    static const Glyph5x7 k_S = {{ 14, 16, 16, 14,  1,  1, 14 }};
    static const Glyph5x7 k_T = {{ 31,  4,  4,  4,  4,  4,  4 }};
    static const Glyph5x7 k_O = {{ 14, 17, 17, 17, 17, 17, 14 }};
    static const Glyph5x7 k_V = {{ 17, 17, 17, 17, 10, 10,  4 }};
    static const Glyph5x7 k_W = {{ 17, 17, 17, 21, 21, 27, 17 }};

    switch (ch)
    {
        case ' ':  return &k_Space;
        case '-':  return &k_Hyphen;
        case 'A':  return &k_A;
        case 'B':  return &k_B;
        case 'C':  return &k_C;
        case 'D':  return &k_D;
        case 'E':  return &k_E;
        case 'H':  return &k_H;
        case 'I':  return &k_I;
        case 'K':  return &k_K;
        case 'L':  return &k_L;
        case 'M':  return &k_M;
        case 'N':  return &k_N;
        case 'O':  return &k_O;
        case 'R':  return &k_R;
        case 'S':  return &k_S;
        case 'T':  return &k_T;
        case 'V':  return &k_V;
        case 'W':  return &k_W;
        default:   return &k_Blank;
    }
}

// Draw one 5×7 glyph at pixel (x, y) with each pixel scaled to scale×scale.
static void DrawGlyph(RendererState* rs, int32 x, int32 y, int32 scale,
                      const Glyph5x7* g, Pixel color)
{
    for (int32 row = 0; row < 7; ++row)
    {
        uint8 mask = g->rows[row];
        for (int32 col = 0; col < 5; ++col)
        {
            if ((mask >> (4 - col)) & 1)
                DrawRect(rs, x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

// Draw a null-terminated ASCII string starting at pixel (x, y).
// Characters are rendered at the given scale; each character advances by (5+1)*scale px.
static void DrawStatusText(RendererState* rs, const uint8* text,
                           int32 x, int32 y, int32 scale, Pixel color)
{
    int32 char_step = (5 + 1) * scale;
    int32 cx = x;
    for (const uint8* p = text; *p; ++p)
    {
        DrawGlyph(rs, cx, y, scale, GetGlyph(*p), color);
        cx += char_step;
    }
}

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

    // Determine once whether the side to move is in check (used for king highlight).
    bool in_check = IsInCheck(&gs->board, gs->side_to_move);

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

            // Check highlight: king square of the side to move when in check.
            // Applied before the selection overlay so the selection can override it.
            if (in_check)
            {
                const Square& sq = gs->board.squares[rank][file];
                if (sq.piece == PIECE_KING && sq.color == gs->side_to_move)
                    sq_color = BOARD_CHECK;
            }

            // Overlay: selected square or valid-move target (overrides check highlight)
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

// ---------------------------------------------------------------------------
// Status overlay: checkmate and stalemate end-game messages.
// ---------------------------------------------------------------------------

void DrawStatusOverlay(RendererState*   rs,
                       const GameState* gs,
                       int32            board_x,
                       int32            board_y,
                       int32            square_size)
{
    ASSERT(rs && gs);

    GameResult result = EvaluatePosition(gs);
    if (result == GAME_ONGOING) return;

    const uint8* msg = nullptr;
    if (result == GAME_WHITE_WINS)
        msg = (const uint8*)"CHECKMATE - WHITE WINS";
    else if (result == GAME_BLACK_WINS)
        msg = (const uint8*)"CHECKMATE - BLACK WINS";
    else
        msg = (const uint8*)"STALEMATE - DRAW";

    // Measure text width: N chars × (5+1)*scale − scale (no trailing gap on last char).
    int32 text_scale = 3;
    int32 char_step  = (5 + 1) * text_scale;   // 18 px per character
    int32 glyph_h    = 7 * text_scale;           // 21 px
    int32 banner_pad = text_scale * 2;            // 6 px vertical padding

    int32 text_len = 0;
    for (const uint8* p = msg; *p; ++p) ++text_len;
    int32 text_w = text_len * char_step - text_scale; // last char has no trailing gap

    int32 board_px  = 8 * square_size;
    int32 banner_h  = glyph_h + banner_pad * 2;
    int32 banner_y  = board_y + (board_px - banner_h) / 2;
    int32 text_x    = board_x + (board_px - text_w) / 2;
    int32 text_y    = banner_y + banner_pad;

    // Dark banner spanning the full board width.
    DrawRect(rs, board_x, banner_y, board_px, banner_h, STATUS_BANNER_BG);

    // Message text centered inside the banner.
    DrawStatusText(rs, msg, text_x, text_y, text_scale, STATUS_TEXT_COLOR);
}

// ---------------------------------------------------------------------------
// Promotion picker
// ---------------------------------------------------------------------------

// Promotion piece order — must match the order used in input.cpp.
static const PieceType k_PickerPieces[4] = {
    PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT
};

// Golden highlight for the promotion picker squares.
static const Pixel BOARD_PROMO_PICK = { 0, 190, 240, 0 };  // BGR ≈ #F0BE00

void DrawPromotionPicker(RendererState*   rs,
                         int32            board_x,
                         int32            board_y,
                         int32            square_size,
                         int8             to_rank,
                         int8             to_file,
                         Color            promoting_side)
{
    ASSERT(rs);

    for (int32 i = 0; i < 4; ++i)
    {
        // White: picker descends from the promotion rank (rank 7).
        // Black: picker ascends from the promotion rank (rank 0).
        int8 picker_rank = (promoting_side == COLOR_WHITE)
                            ? (int8)(to_rank - i)
                            : (int8)(to_rank + i);

        int32 sq_x = board_x + to_file  * square_size;
        int32 sq_y = board_y + (7 - picker_rank) * square_size;

        DrawRect(rs, sq_x, sq_y, square_size, square_size, BOARD_PROMO_PICK);
        DrawPiece(rs, sq_x, sq_y, square_size, k_PickerPieces[i], promoting_side);
    }
}

// ---------------------------------------------------------------------------
// Game-over overlay
// ---------------------------------------------------------------------------

// Layout of the game-over panel, expressed as fractions of the 8-square board.
// The panel is centered inside the board area.
static const int32 OVERLAY_W  = 320;  // panel width  (4 squares)
static const int32 OVERLAY_H  = 160;  // panel height (2 squares)
static const int32 RESULT_H   =  60;  // result-strip height inside the panel
static const int32 BTN_MARGIN =  20;  // inset of the restart button from the panel edges
static const int32 BTN_H      =  60;  // restart button height

// Panel colours.
static const Pixel OVERLAY_BG       = {  20,  20,  20, 0 };  // near-black background
static const Pixel OVERLAY_BORDER   = { 180, 180, 180, 0 };  // light-gray border

static const Pixel RESULT_WHITE_COL = { 220, 235, 235, 0 };  // near-white  — White wins
static const Pixel RESULT_BLACK_COL = {  50,  50,  50, 0 };  // dark gray   — Black wins
static const Pixel RESULT_DRAW_COL  = { 120, 120, 120, 0 };  // mid-gray    — draw

static const Pixel BTN_FILL         = {  50, 180,  50, 0 };  // green restart button
static const Pixel BTN_BORDER_COLOR = { 200, 240, 200, 0 };  // light-green button border

// Derive the top-left pixel of the overlay panel, centered on the board.
static inline void GetOverlayTopLeft(int32 board_x, int32 board_y, int32 square_size,
                                     int32* ox, int32* oy)
{
    int32 board_px = square_size * 8;
    *ox = board_x + (board_px - OVERLAY_W) / 2;
    *oy = board_y + (board_px - OVERLAY_H) / 2;
}

// Derive the pixel rect of the restart button.
static inline void GetRestartButtonRect(int32 board_x, int32 board_y, int32 square_size,
                                        int32* bx, int32* by, int32* bw, int32* bh)
{
    int32 ox, oy;
    GetOverlayTopLeft(board_x, board_y, square_size, &ox, &oy);
    *bx = ox + BTN_MARGIN;
    *by = oy + RESULT_H + BTN_MARGIN;
    *bw = OVERLAY_W - BTN_MARGIN * 2;
    *bh = BTN_H;
}

void DrawGameOverOverlay(RendererState*   rs,
                         GameResult       result,
                         int32            board_x,
                         int32            board_y,
                         int32            square_size)
{
    ASSERT(rs);

    int32 ox, oy;
    GetOverlayTopLeft(board_x, board_y, square_size, &ox, &oy);

    // Outer border then dark background.
    DrawRect(rs, ox - 2, oy - 2, OVERLAY_W + 4, OVERLAY_H + 4, OVERLAY_BORDER);
    DrawRect(rs, ox, oy, OVERLAY_W, OVERLAY_H, OVERLAY_BG);

    // Result strip: colour indicates who won.
    Pixel result_color;
    Color winner_color;
    switch (result)
    {
        case GAME_WHITE_WINS:
            result_color = RESULT_WHITE_COL;
            winner_color = COLOR_WHITE;
            break;
        case GAME_BLACK_WINS:
            result_color = RESULT_BLACK_COL;
            winner_color = COLOR_BLACK;
            break;
        default: // GAME_DRAW
            result_color = RESULT_DRAW_COL;
            winner_color = COLOR_NONE;
            break;
    }
    DrawRect(rs, ox, oy, OVERLAY_W, RESULT_H, result_color);

    // Draw a king icon centred in the result strip to hint at the winner.
    // For a draw, draw both kings on the left and right sides of the strip.
    int32 icon_size = RESULT_H;

    if (winner_color != COLOR_NONE)
    {
        int32 sq_x = ox + (OVERLAY_W - icon_size) / 2;
        DrawPiece(rs, sq_x, oy, icon_size, PIECE_KING, winner_color);
    }
    else
    {
        DrawPiece(rs, ox,                           oy, icon_size, PIECE_KING, COLOR_WHITE);
        DrawPiece(rs, ox + OVERLAY_W - icon_size,   oy, icon_size, PIECE_KING, COLOR_BLACK);
    }

    // Render the result message text inside the result strip at scale 2.
    // Text color contrasts with the strip: dark on a light strip, light on dark/mid.
    {
        const uint8* msg;
        Pixel msg_color;
        if (result == GAME_WHITE_WINS)
        {
            msg       = (const uint8*)"CHECKMATE - WHITE WINS";
            msg_color = OVERLAY_BG;
        }
        else if (result == GAME_BLACK_WINS)
        {
            msg       = (const uint8*)"CHECKMATE - BLACK WINS";
            msg_color = STATUS_TEXT_COLOR;
        }
        else
        {
            msg       = (const uint8*)"STALEMATE - DRAW";
            msg_color = STATUS_TEXT_COLOR;
        }

        int32 text_scale = 2;
        int32 char_step  = (5 + 1) * text_scale;  // 12 px per character
        int32 glyph_h2   = 7 * text_scale;         // 14 px

        int32 msg_len = 0;
        for (const uint8* p = msg; *p; ++p) ++msg_len;
        int32 msg_w = msg_len * char_step - text_scale;

        int32 msg_x = ox + (OVERLAY_W - msg_w) / 2;
        int32 msg_y = oy + (RESULT_H - glyph_h2) / 2;

        DrawStatusText(rs, msg, msg_x, msg_y, text_scale, msg_color);
    }

    // Restart button: green rectangle.
    int32 bx, by, bw, bh;
    GetRestartButtonRect(board_x, board_y, square_size, &bx, &by, &bw, &bh);
    DrawRect(rs, bx - 2, by - 2, bw + 4, bh + 4, BTN_BORDER_COLOR);
    DrawRect(rs, bx, by, bw, bh, BTN_FILL);

    // Draw a ring icon inside the button as a simple "refresh" indicator.
    int32 btn_cx = bx + bw / 2;
    int32 btn_cy = by + bh / 2;
    int32 icon_r = bh * 28 / 100;
    DrawFilledCircle(rs, btn_cx, btn_cy, icon_r + 3, BTN_BORDER_COLOR);
    DrawFilledCircle(rs, btn_cx, btn_cy, icon_r,     BTN_FILL);
    int32 inner_r = icon_r * 55 / 100;
    DrawFilledCircle(rs, btn_cx, btn_cy, inner_r, BTN_FILL);
}

bool IsRestartButtonHit(int32 px, int32 py,
                        int32 board_x, int32 board_y, int32 square_size)
{
    int32 bx, by, bw, bh;
    GetRestartButtonRect(board_x, board_y, square_size, &bx, &by, &bw, &bh);
    return px >= bx && px < bx + bw && py >= by && py < by + bh;
}

// ---------------------------------------------------------------------------
// Turn indicator
// ---------------------------------------------------------------------------

void DrawTurnIndicator(RendererState* rs, Color side_to_move,
                       int32 board_x, int32 board_y, int32 square_size)
{
    ASSERT(rs);

    const uint8* msg = (side_to_move == COLOR_WHITE)
                       ? (const uint8*)"WHITE TO MOVE"
                       : (const uint8*)"BLACK TO MOVE";

    int32 text_scale = 2;
    int32 char_step  = (5 + 1) * text_scale;  // 12 px per character
    int32 glyph_h    = 7 * text_scale;         // 14 px
    int32 pad        = text_scale * 2;         // 4 px vertical padding

    int32 text_len = 0;
    for (const uint8* p = msg; *p; ++p) ++text_len;
    int32 text_w = text_len * char_step - text_scale;

    int32 board_px = 8 * square_size;
    int32 bar_h    = glyph_h + pad * 2;
    // Centre the bar in the gap below the board.  The gap equals board_y for
    // the symmetric 1280x720 layout (board_y pixels above and below).
    int32 bar_y    = board_y + board_px + (board_y - bar_h) / 2;
    int32 text_x   = board_x + (board_px - text_w) / 2;
    int32 text_y   = bar_y + pad;

    DrawRect(rs, board_x, bar_y, board_px, bar_h, STATUS_BANNER_BG);
    DrawStatusText(rs, msg, text_x, text_y, text_scale, STATUS_TEXT_COLOR);
}
