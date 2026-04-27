#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "moves.h"

void InitGameState(GameState* gs)
{
    ASSERT(gs);
    InitBoard(&gs->board);
    gs->side_to_move    = COLOR_WHITE;
    gs->en_passant_rank = -1;
    gs->en_passant_file = -1;
    gs->castling_wk     = true;
    gs->castling_wq     = true;
    gs->castling_bk     = true;
    gs->castling_bq     = true;
}

void GeneratePawnMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    const Color  color      = gs->side_to_move;
    const Board* board      = &gs->board;
    const int8   dir        = (color == COLOR_WHITE) ? 1 : -1;
    const int8   start_rank = (color == COLOR_WHITE) ? 1 : 6;
    const int8   promo_rank = (color == COLOR_WHITE) ? 7 : 0;
    const Color  enemy      = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;

    static const PieceType k_promo_pieces[] =
        { PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT };

    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            const Square sq = board->squares[rank][file];
            if (sq.piece != PIECE_PAWN || sq.color != color) continue;

            const int8 r         = (int8)rank;
            const int8 f         = (int8)file;
            const int8 next_rank = r + dir;

            // Single push forward.
            if (next_rank >= 0 && next_rank < 8 &&
                board->squares[next_rank][f].piece == PIECE_NONE)
            {
                if (next_rank == promo_rank)
                {
                    // Emit one move per promotion piece.
                    for (int32 pi = 0; pi < 4; ++pi)
                    {
                        ASSERT(list->count < MAX_MOVES_PER_POSITION);
                        Move& pm      = list->moves[list->count++];
                        pm.from_rank  = r;
                        pm.from_file  = f;
                        pm.to_rank    = next_rank;
                        pm.to_file    = f;
                        pm.promotion  = k_promo_pieces[pi];
                        pm.is_en_passant = false;
                        pm.is_castling   = false;
                    }
                }
                else
                {
                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& m      = list->moves[list->count++];
                    m.from_rank  = r;
                    m.from_file  = f;
                    m.to_rank    = next_rank;
                    m.to_file    = f;
                    m.promotion  = PIECE_NONE;
                    m.is_en_passant = false;
                    m.is_castling   = false;

                    // Double push from starting rank (only when single push path is clear).
                    const int8 double_rank = r + (int8)(2 * dir);
                    if (r == start_rank &&
                        board->squares[double_rank][f].piece == PIECE_NONE)
                    {
                        ASSERT(list->count < MAX_MOVES_PER_POSITION);
                        Move& dm      = list->moves[list->count++];
                        dm.from_rank  = r;
                        dm.from_file  = f;
                        dm.to_rank    = double_rank;
                        dm.to_file    = f;
                        dm.promotion  = PIECE_NONE;
                        dm.is_en_passant = false;
                        dm.is_castling   = false;
                    }
                }
            }

            // Diagonal captures and en passant.
            const int8 cap_files[2] = { f - 1, f + 1 };
            for (int32 i = 0; i < 2; ++i)
            {
                const int8 cf = cap_files[i];
                if (cf < 0 || cf >= 8)           continue;
                if (next_rank < 0 || next_rank >= 8) continue;

                const Square target = board->squares[next_rank][cf];

                // Normal diagonal capture (including capture-promotion).
                if (target.piece != PIECE_NONE && target.color == enemy)
                {
                    if (next_rank == promo_rank)
                    {
                        for (int32 pi = 0; pi < 4; ++pi)
                        {
                            ASSERT(list->count < MAX_MOVES_PER_POSITION);
                            Move& cm      = list->moves[list->count++];
                            cm.from_rank  = r;
                            cm.from_file  = f;
                            cm.to_rank    = next_rank;
                            cm.to_file    = cf;
                            cm.promotion  = k_promo_pieces[pi];
                            cm.is_en_passant = false;
                            cm.is_castling   = false;
                        }
                    }
                    else
                    {
                        ASSERT(list->count < MAX_MOVES_PER_POSITION);
                        Move& cm      = list->moves[list->count++];
                        cm.from_rank  = r;
                        cm.from_file  = f;
                        cm.to_rank    = next_rank;
                        cm.to_file    = cf;
                        cm.promotion  = PIECE_NONE;
                        cm.is_en_passant = false;
                        cm.is_castling   = false;
                    }
                }

                // En passant capture — always for side_to_move (color == gs->side_to_move).
                if (gs->en_passant_rank == next_rank && gs->en_passant_file == cf)
                {
                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& ep      = list->moves[list->count++];
                    ep.from_rank  = r;
                    ep.from_file  = f;
                    ep.to_rank    = next_rank;
                    ep.to_file    = cf;
                    ep.promotion  = PIECE_NONE;
                    ep.is_en_passant = true;
                    ep.is_castling   = false;
                }
            }
        }
    }
}

void GenerateKnightMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    const Color  color = gs->side_to_move;
    const Board* board = &gs->board;

    static const int8 k_offsets[8][2] =
    {
        { 2,  1 }, { 2, -1 },
        {-2,  1 }, {-2, -1 },
        { 1,  2 }, { 1, -2 },
        {-1,  2 }, {-1, -2 },
    };

    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            const Square sq = board->squares[rank][file];
            if (sq.piece != PIECE_KNIGHT || sq.color != color) continue;

            const int8 r = (int8)rank;
            const int8 f = (int8)file;

            for (int32 i = 0; i < 8; ++i)
            {
                const int8 tr = r + k_offsets[i][0];
                const int8 tf = f + k_offsets[i][1];

                if (tr < 0 || tr >= 8) continue;
                if (tf < 0 || tf >= 8) continue;

                const Square target = board->squares[tr][tf];
                if (target.piece != PIECE_NONE && target.color == color) continue;

                ASSERT(list->count < MAX_MOVES_PER_POSITION);
                Move& m      = list->moves[list->count++];
                m.from_rank  = r;
                m.from_file  = f;
                m.to_rank    = tr;
                m.to_file    = tf;
                m.promotion  = PIECE_NONE;
                m.is_en_passant = false;
                m.is_castling   = false;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Shared ray-casting helper: cast rays from each piece of the given type and
// the given set of directions. For each step along the ray, stop (exclusive)
// before a friendly piece or stop (inclusive) after an enemy piece.
// ---------------------------------------------------------------------------
static void CastRays(const GameState* gs, MoveList* list, PieceType piece_type,
                     const int8 dirs[][2], int32 dir_count)
{
    const Color  color = gs->side_to_move;
    const Board* board = &gs->board;

    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            const Square sq = board->squares[rank][file];
            if (sq.piece != piece_type || sq.color != color) continue;

            const int8 r = (int8)rank;
            const int8 f = (int8)file;

            for (int32 d = 0; d < dir_count; ++d)
            {
                int8 tr = r + dirs[d][0];
                int8 tf = f + dirs[d][1];

                while (tr >= 0 && tr < 8 && tf >= 0 && tf < 8)
                {
                    const Square target = board->squares[tr][tf];

                    if (target.piece != PIECE_NONE && target.color == color)
                    {
                        // Friendly piece — stop ray, do not include this square.
                        break;
                    }

                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& m      = list->moves[list->count++];
                    m.from_rank  = r;
                    m.from_file  = f;
                    m.to_rank    = tr;
                    m.to_file    = tf;
                    m.promotion  = PIECE_NONE;
                    m.is_en_passant = false;
                    m.is_castling   = false;

                    if (target.piece != PIECE_NONE)
                    {
                        // Enemy piece — include this capture square and stop ray.
                        break;
                    }

                    tr = (int8)(tr + dirs[d][0]);
                    tf = (int8)(tf + dirs[d][1]);
                }
            }
        }
    }
}

void GenerateRookMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    static const int8 k_rook_dirs[4][2] =
    {
        { 1,  0 }, { -1,  0 },
        { 0,  1 }, {  0, -1 },
    };

    CastRays(gs, list, PIECE_ROOK, k_rook_dirs, 4);
}

void GenerateBishopMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    static const int8 k_bishop_dirs[4][2] =
    {
        { 1,  1 }, { 1, -1 },
        {-1,  1 }, {-1, -1 },
    };

    CastRays(gs, list, PIECE_BISHOP, k_bishop_dirs, 4);
}

void GenerateQueenMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    static const int8 k_queen_dirs[8][2] =
    {
        { 1,  0 }, { -1,  0 },
        { 0,  1 }, {  0, -1 },
        { 1,  1 }, {  1, -1 },
        {-1,  1 }, { -1, -1 },
    };

    CastRays(gs, list, PIECE_QUEEN, k_queen_dirs, 8);
}

// ---------------------------------------------------------------------------
// IsSquareAttacked: returns true if 'rank'/'file' is attacked by any piece
// of 'by_color'. Checks pawns, knights, sliding pieces, and the enemy king.
// ---------------------------------------------------------------------------
bool IsSquareAttacked(const Board* board, int8 rank, int8 file, Color by_color)
{
    ASSERT(board);

    // Pawn attacks: a white pawn attacks diagonally upward (+rank), so this
    // square is attacked by a white pawn sitting one rank below at file±1.
    // A black pawn attacks downward (−rank), so the pawn sits one rank above.
    const int8 pawn_src_rank = (by_color == COLOR_WHITE) ? (int8)(rank - 1) : (int8)(rank + 1);
    if (pawn_src_rank >= 0 && pawn_src_rank < 8)
    {
        const int8 pawn_files[2] = { (int8)(file - 1), (int8)(file + 1) };
        for (int32 i = 0; i < 2; ++i)
        {
            const int8 pf = pawn_files[i];
            if (pf >= 0 && pf < 8)
            {
                const Square sq = board->squares[pawn_src_rank][pf];
                if (sq.piece == PIECE_PAWN && sq.color == by_color)
                    return true;
            }
        }
    }

    // Knight attacks.
    static const int8 k_knight_offs[8][2] =
    {
        { 2,  1 }, { 2, -1 },
        {-2,  1 }, {-2, -1 },
        { 1,  2 }, { 1, -2 },
        {-1,  2 }, {-1, -2 },
    };
    for (int32 i = 0; i < 8; ++i)
    {
        const int8 kr = (int8)(rank + k_knight_offs[i][0]);
        const int8 kf = (int8)(file + k_knight_offs[i][1]);
        if (kr >= 0 && kr < 8 && kf >= 0 && kf < 8)
        {
            const Square sq = board->squares[kr][kf];
            if (sq.piece == PIECE_KNIGHT && sq.color == by_color)
                return true;
        }
    }

    // Orthogonal rays: rook or queen.
    static const int8 k_ortho[4][2] =
    {
        { 1,  0 }, { -1,  0 },
        { 0,  1 }, {  0, -1 },
    };
    for (int32 d = 0; d < 4; ++d)
    {
        int8 r = (int8)(rank + k_ortho[d][0]);
        int8 f = (int8)(file + k_ortho[d][1]);
        while (r >= 0 && r < 8 && f >= 0 && f < 8)
        {
            const Square sq = board->squares[r][f];
            if (sq.piece != PIECE_NONE)
            {
                if (sq.color == by_color &&
                    (sq.piece == PIECE_ROOK || sq.piece == PIECE_QUEEN))
                    return true;
                break;
            }
            r = (int8)(r + k_ortho[d][0]);
            f = (int8)(f + k_ortho[d][1]);
        }
    }

    // Diagonal rays: bishop or queen.
    static const int8 k_diag[4][2] =
    {
        { 1,  1 }, { 1, -1 },
        {-1,  1 }, {-1, -1 },
    };
    for (int32 d = 0; d < 4; ++d)
    {
        int8 r = (int8)(rank + k_diag[d][0]);
        int8 f = (int8)(file + k_diag[d][1]);
        while (r >= 0 && r < 8 && f >= 0 && f < 8)
        {
            const Square sq = board->squares[r][f];
            if (sq.piece != PIECE_NONE)
            {
                if (sq.color == by_color &&
                    (sq.piece == PIECE_BISHOP || sq.piece == PIECE_QUEEN))
                    return true;
                break;
            }
            r = (int8)(r + k_diag[d][0]);
            f = (int8)(f + k_diag[d][1]);
        }
    }

    // King adjacency (prevents kings from standing next to each other).
    static const int8 k_adj[8][2] =
    {
        { 1,  0 }, { -1,  0 },
        { 0,  1 }, {  0, -1 },
        { 1,  1 }, {  1, -1 },
        {-1,  1 }, { -1, -1 },
    };
    for (int32 i = 0; i < 8; ++i)
    {
        const int8 kr = (int8)(rank + k_adj[i][0]);
        const int8 kf = (int8)(file + k_adj[i][1]);
        if (kr >= 0 && kr < 8 && kf >= 0 && kf < 8)
        {
            const Square sq = board->squares[kr][kf];
            if (sq.piece == PIECE_KING && sq.color == by_color)
                return true;
        }
    }

    return false;
}

void GenerateKingMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    const Color  color = gs->side_to_move;
    const Color  enemy = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    const Board* board = &gs->board;

    static const int8 k_king_dirs[8][2] =
    {
        { 1,  0 }, { -1,  0 },
        { 0,  1 }, {  0, -1 },
        { 1,  1 }, {  1, -1 },
        {-1,  1 }, { -1, -1 },
    };

    // Find the king of side_to_move.
    int8 king_rank = -1;
    int8 king_file = -1;
    for (int32 rank = 0; rank < 8 && king_rank < 0; ++rank)
    {
        for (int32 file = 0; file < 8 && king_rank < 0; ++file)
        {
            const Square sq = board->squares[rank][file];
            if (sq.piece == PIECE_KING && sq.color == color)
            {
                king_rank = (int8)rank;
                king_file = (int8)file;
            }
        }
    }

    if (king_rank < 0) return; // No king on the board (should not happen in a real game).

    // Normal moves to all 8 adjacent squares.
    for (int32 i = 0; i < 8; ++i)
    {
        const int8 tr = (int8)(king_rank + k_king_dirs[i][0]);
        const int8 tf = (int8)(king_file + k_king_dirs[i][1]);

        if (tr < 0 || tr >= 8) continue;
        if (tf < 0 || tf >= 8) continue;

        const Square target = board->squares[tr][tf];
        if (target.piece != PIECE_NONE && target.color == color) continue;
        if (IsSquareAttacked(board, tr, tf, enemy)) continue;

        ASSERT(list->count < MAX_MOVES_PER_POSITION);
        Move& m         = list->moves[list->count++];
        m.from_rank     = king_rank;
        m.from_file     = king_file;
        m.to_rank       = tr;
        m.to_file       = tf;
        m.promotion     = PIECE_NONE;
        m.is_en_passant = false;
        m.is_castling   = false;
    }

    // Castling — only possible when the king is on its home square.
    const int8 home_rank = (color == COLOR_WHITE) ? 0 : 7;
    if (king_rank != home_rank || king_file != 4) return;

    // Kingside castling (king travels e→f→g; rook is on h).
    const bool ks_right = (color == COLOR_WHITE) ? gs->castling_wk : gs->castling_bk;
    if (ks_right &&
        board->squares[home_rank][5].piece == PIECE_NONE &&
        board->squares[home_rank][6].piece == PIECE_NONE &&
        !IsSquareAttacked(board, home_rank, 4, enemy) &&
        !IsSquareAttacked(board, home_rank, 5, enemy) &&
        !IsSquareAttacked(board, home_rank, 6, enemy))
    {
        ASSERT(list->count < MAX_MOVES_PER_POSITION);
        Move& m         = list->moves[list->count++];
        m.from_rank     = king_rank;
        m.from_file     = king_file;
        m.to_rank       = home_rank;
        m.to_file       = 6;
        m.promotion     = PIECE_NONE;
        m.is_en_passant = false;
        m.is_castling   = true;
    }

    // Queenside castling (king travels e→d→c; rook is on a; b-file must also be clear).
    const bool qs_right = (color == COLOR_WHITE) ? gs->castling_wq : gs->castling_bq;
    if (qs_right &&
        board->squares[home_rank][1].piece == PIECE_NONE &&
        board->squares[home_rank][2].piece == PIECE_NONE &&
        board->squares[home_rank][3].piece == PIECE_NONE &&
        !IsSquareAttacked(board, home_rank, 4, enemy) &&
        !IsSquareAttacked(board, home_rank, 3, enemy) &&
        !IsSquareAttacked(board, home_rank, 2, enemy))
    {
        ASSERT(list->count < MAX_MOVES_PER_POSITION);
        Move& m         = list->moves[list->count++];
        m.from_rank     = king_rank;
        m.from_file     = king_file;
        m.to_rank       = home_rank;
        m.to_file       = 2;
        m.promotion     = PIECE_NONE;
        m.is_en_passant = false;
        m.is_castling   = true;
    }
}

void ApplyMove(GameState* gs, const Move* move){
    ASSERT(gs);
    ASSERT(move);

    Board*       board        = &gs->board;
    const Square moving_piece = board->squares[move->from_rank][move->from_file];

    // Clear en passant; will be re-set below if this is a double pawn push.
    gs->en_passant_rank = -1;
    gs->en_passant_file = -1;

    // En passant capture: remove the pawn that was jumped over.
    // The captured pawn sits on (from_rank, to_file) — same rank as attacker, target file.
    if (move->is_en_passant)
    {
        board->squares[move->from_rank][move->to_file] = { PIECE_NONE, COLOR_NONE };
    }

    // Move the piece to the destination square.
    board->squares[move->to_rank][move->to_file]     = moving_piece;
    board->squares[move->from_rank][move->from_file] = { PIECE_NONE, COLOR_NONE };

    // Promotion: replace pawn with the promoted piece type.
    if (move->promotion != PIECE_NONE)
    {
        board->squares[move->to_rank][move->to_file].piece = move->promotion;
    }

    // Castling: also relocate the rook.
    if (move->is_castling)
    {
        const int8 home_rank = move->from_rank;
        if (move->to_file == 6) // Kingside: rook h→f
        {
            board->squares[home_rank][5] = board->squares[home_rank][7];
            board->squares[home_rank][7] = { PIECE_NONE, COLOR_NONE };
        }
        else // Queenside (to_file == 2): rook a→d
        {
            board->squares[home_rank][3] = board->squares[home_rank][0];
            board->squares[home_rank][0] = { PIECE_NONE, COLOR_NONE };
        }
    }

    // Double pawn push: record the skipped square as the en passant target.
    if (moving_piece.piece == PIECE_PAWN)
    {
        const int8 rank_diff = move->to_rank - move->from_rank;
        if (rank_diff == 2 || rank_diff == -2)
        {
            gs->en_passant_rank = (move->from_rank + move->to_rank) / 2;
            gs->en_passant_file = move->from_file;
        }
    }

    // Revoke castling rights when the king or a rook moves.
    if (moving_piece.piece == PIECE_KING)
    {
        if (moving_piece.color == COLOR_WHITE)
        {
            gs->castling_wk = false;
            gs->castling_wq = false;
        }
        else
        {
            gs->castling_bk = false;
            gs->castling_bq = false;
        }
    }
    if (moving_piece.piece == PIECE_ROOK)
    {
        if (move->from_rank == 0 && move->from_file == 0) gs->castling_wq = false;
        if (move->from_rank == 0 && move->from_file == 7) gs->castling_wk = false;
        if (move->from_rank == 7 && move->from_file == 0) gs->castling_bq = false;
        if (move->from_rank == 7 && move->from_file == 7) gs->castling_bk = false;
    }

    // Revoke castling rights when a rook is captured on its home square.
    if (move->to_rank == 0 && move->to_file == 0) gs->castling_wq = false;
    if (move->to_rank == 0 && move->to_file == 7) gs->castling_wk = false;
    if (move->to_rank == 7 && move->to_file == 0) gs->castling_bq = false;
    if (move->to_rank == 7 && move->to_file == 7) gs->castling_bk = false;

    // Advance turn.
    gs->side_to_move = (gs->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
}
