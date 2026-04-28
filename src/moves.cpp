#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "memory.h"
#include "moves.h"

void InitGameState(GameState* gs)
{
    ASSERT(gs);
    InitBoard(&gs->board);
    gs->side_to_move             = COLOR_WHITE;
    gs->en_passant_rank          = -1;
    gs->en_passant_file          = -1;
    gs->castling_white_kingside  = true;
    gs->castling_white_queenside = true;
    gs->castling_black_kingside  = true;
    gs->castling_black_queenside = true;
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
                        Move& pm         = list->moves[list->count++];
                        pm.from_rank     = r;
                        pm.from_file     = f;
                        pm.to_rank       = next_rank;
                        pm.to_file       = f;
                        pm.promotion     = k_promo_pieces[pi];
                        pm.is_en_passant = false;
                        pm.is_castling   = false;
                    }
                }
                else
                {
                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& m          = list->moves[list->count++];
                    m.from_rank      = r;
                    m.from_file      = f;
                    m.to_rank        = next_rank;
                    m.to_file        = f;
                    m.promotion      = PIECE_NONE;
                    m.is_en_passant  = false;
                    m.is_castling    = false;

                    // Double push from starting rank (only when single push path is clear).
                    const int8 double_rank = r + (int8)(2 * dir);
                    if (r == start_rank &&
                        board->squares[double_rank][f].piece == PIECE_NONE)
                    {
                        ASSERT(list->count < MAX_MOVES_PER_POSITION);
                        Move& dm         = list->moves[list->count++];
                        dm.from_rank     = r;
                        dm.from_file     = f;
                        dm.to_rank       = double_rank;
                        dm.to_file       = f;
                        dm.promotion     = PIECE_NONE;
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
                            Move& cm         = list->moves[list->count++];
                            cm.from_rank     = r;
                            cm.from_file     = f;
                            cm.to_rank       = next_rank;
                            cm.to_file       = cf;
                            cm.promotion     = k_promo_pieces[pi];
                            cm.is_en_passant = false;
                            cm.is_castling   = false;
                        }
                    }
                    else
                    {
                        ASSERT(list->count < MAX_MOVES_PER_POSITION);
                        Move& cm         = list->moves[list->count++];
                        cm.from_rank     = r;
                        cm.from_file     = f;
                        cm.to_rank       = next_rank;
                        cm.to_file       = cf;
                        cm.promotion     = PIECE_NONE;
                        cm.is_en_passant = false;
                        cm.is_castling   = false;
                    }
                }

                // En passant capture — always for side_to_move (color == gs->side_to_move).
                if (gs->en_passant_rank == next_rank && gs->en_passant_file == cf)
                {
                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& ep         = list->moves[list->count++];
                    ep.from_rank     = r;
                    ep.from_file     = f;
                    ep.to_rank       = next_rank;
                    ep.to_file       = cf;
                    ep.promotion     = PIECE_NONE;
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
                Move& m          = list->moves[list->count++];
                m.from_rank      = r;
                m.from_file      = f;
                m.to_rank        = tr;
                m.to_file        = tf;
                m.promotion      = PIECE_NONE;
                m.is_en_passant  = false;
                m.is_castling    = false;
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
                    Move& m          = list->moves[list->count++];
                    m.from_rank      = r;
                    m.from_file      = f;
                    m.to_rank        = tr;
                    m.to_file        = tf;
                    m.promotion      = PIECE_NONE;
                    m.is_en_passant  = false;
                    m.is_castling    = false;

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

void GenerateKingMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    const Color  color = gs->side_to_move;
    const Board* board = &gs->board;

    static const int8 k_king_offsets[8][2] =
    {
        { 1,  0 }, { -1,  0 },
        { 0,  1 }, {  0, -1 },
        { 1,  1 }, {  1, -1 },
        {-1,  1 }, { -1, -1 },
    };

    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            const Square sq = board->squares[rank][file];
            if (sq.piece != PIECE_KING || sq.color != color) continue;

            const int8 r = (int8)rank;
            const int8 f = (int8)file;

            for (int32 i = 0; i < 8; ++i)
            {
                const int8 tr = r + k_king_offsets[i][0];
                const int8 tf = f + k_king_offsets[i][1];

                if (tr < 0 || tr >= 8) continue;
                if (tf < 0 || tf >= 8) continue;

                const Square target = board->squares[tr][tf];
                if (target.piece != PIECE_NONE && target.color == color) continue;

                ASSERT(list->count < MAX_MOVES_PER_POSITION);
                Move& m          = list->moves[list->count++];
                m.from_rank      = r;
                m.from_file      = f;
                m.to_rank        = tr;
                m.to_file        = tf;
                m.promotion      = PIECE_NONE;
                m.is_en_passant  = false;
                m.is_castling    = false;
            }
        }
    }
}


// Returns true if (rank, file) is attacked by any piece of 'attacker' on board.
// Pawn attacks use direct diagonal detection only (no forward pushes).
// Pinned pieces are NOT filtered — a piece attacks its normal squares even when
// moving there would expose its own king (FIDE Article 3.8).
static bool IsSquareAttackedBy(const Board* board, int8 rank, int8 file, Color attacker)
{
    // --- Pawn attack check (diagonal only) ---
    {
        const int8 pawn_dir = (attacker == COLOR_WHITE) ? -1 : 1; // direction from target back to attacker
        const int8 pr = (int8)(rank + pawn_dir);
        if (pr >= 0 && pr < 8)
        {
            for (int32 df = -1; df <= 1; df += 2)
            {
                const int8 pf = (int8)(file + df);
                if (pf >= 0 && pf < 8)
                {
                    const Square sq = board->squares[pr][pf];
                    if (sq.piece == PIECE_PAWN && sq.color == attacker)
                        return true;
                }
            }
        }
    }

    // --- All other pieces via pseudo-legal move generation ---
    GameState temp_gs;
    temp_gs.board                    = *board;
    temp_gs.side_to_move             = attacker;
    temp_gs.en_passant_rank          = -1;
    temp_gs.en_passant_file          = -1;
    temp_gs.castling_white_kingside  = false;
    temp_gs.castling_white_queenside = false;
    temp_gs.castling_black_kingside  = false;
    temp_gs.castling_black_queenside = false;

    MoveList attacks = {};
    GenerateKnightMoves(&temp_gs, &attacks);
    GenerateRookMoves  (&temp_gs, &attacks);
    GenerateBishopMoves(&temp_gs, &attacks);
    GenerateQueenMoves (&temp_gs, &attacks);
    GenerateKingMoves  (&temp_gs, &attacks);

    for (int32 i = 0; i < attacks.count; ++i)
    {
        if (attacks.moves[i].to_rank == rank &&
            attacks.moves[i].to_file == file)
        {
            return true;
        }
    }
    return false;
}

void GenerateCastlingMoves(const GameState* gs, MoveList* list)
{
    ASSERT(gs);
    ASSERT(list);

    const Color  color     = gs->side_to_move;
    const Board* board     = &gs->board;
    const Color  enemy     = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    const int8   back_rank = (color == COLOR_WHITE) ? 0 : 7;

    // Helper: castle if rights are set, the pieces are in position, path is clear,
    // and the king neither starts, passes through, nor lands on an attacked square.

    // Kingside castling: king from e-file (4) to g-file (6); rook from h-file (7) to f-file (5).
    const bool ks_right = (color == COLOR_WHITE) ? gs->castling_white_kingside
                                                  : gs->castling_black_kingside;
    if (ks_right)
    {
        if (board->squares[back_rank][4].piece == PIECE_KING &&
            board->squares[back_rank][4].color == color      &&
            board->squares[back_rank][7].piece == PIECE_ROOK &&
            board->squares[back_rank][7].color == color)
        {
            // f and g files must be empty.
            if (board->squares[back_rank][5].piece == PIECE_NONE &&
                board->squares[back_rank][6].piece == PIECE_NONE)
            {
                // King must not start in check, pass through check, or land in check.
                if (!IsSquareAttackedBy(board, back_rank, 4, enemy) &&
                    !IsSquareAttackedBy(board, back_rank, 5, enemy) &&
                    !IsSquareAttackedBy(board, back_rank, 6, enemy))
                {
                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& m         = list->moves[list->count++];
                    m.from_rank     = back_rank;
                    m.from_file     = 4;
                    m.to_rank       = back_rank;
                    m.to_file       = 6;
                    m.promotion     = PIECE_NONE;
                    m.is_en_passant = false;
                    m.is_castling   = true;
                }
            }
        }
    }

    // Queenside castling: king from e-file (4) to c-file (2); rook from a-file (0) to d-file (3).
    const bool qs_right = (color == COLOR_WHITE) ? gs->castling_white_queenside
                                                  : gs->castling_black_queenside;
    if (qs_right)
    {
        if (board->squares[back_rank][4].piece == PIECE_KING &&
            board->squares[back_rank][4].color == color      &&
            board->squares[back_rank][0].piece == PIECE_ROOK &&
            board->squares[back_rank][0].color == color)
        {
            // d, c, and b files must be empty.
            if (board->squares[back_rank][3].piece == PIECE_NONE &&
                board->squares[back_rank][2].piece == PIECE_NONE &&
                board->squares[back_rank][1].piece == PIECE_NONE)
            {
                // King must not start in check, pass through d-file, or land on c-file under attack.
                // (The b-file only needs to be empty, not unattacked.)
                if (!IsSquareAttackedBy(board, back_rank, 4, enemy) &&
                    !IsSquareAttackedBy(board, back_rank, 3, enemy) &&
                    !IsSquareAttackedBy(board, back_rank, 2, enemy))
                {
                    ASSERT(list->count < MAX_MOVES_PER_POSITION);
                    Move& m         = list->moves[list->count++];
                    m.from_rank     = back_rank;
                    m.from_file     = 4;
                    m.to_rank       = back_rank;
                    m.to_file       = 2;
                    m.promotion     = PIECE_NONE;
                    m.is_en_passant = false;
                    m.is_castling   = true;
                }
            }
        }
    }
}

void ApplyMove(GameState* gs, const Move* move){
    ASSERT(gs);
    ASSERT(move);

    Board*       board        = &gs->board;
    const Square moving_piece = board->squares[move->from_rank][move->from_file];
    const Square captured     = board->squares[move->to_rank][move->to_file];

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

    // Castling: also slide the rook to its new square.
    // Kingside: rook from h-file (7) to f-file (5).
    // Queenside: rook from a-file (0) to d-file (3).
    if (move->is_castling)
    {
        const int8 rook_from_file = (move->to_file == 6) ? 7 : 0;
        const int8 rook_to_file   = (move->to_file == 6) ? 5 : 3;
        board->squares[move->to_rank][rook_to_file]   = board->squares[move->to_rank][rook_from_file];
        board->squares[move->to_rank][rook_from_file] = { PIECE_NONE, COLOR_NONE };
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

    // Clear castling rights when the king moves.
    if (moving_piece.piece == PIECE_KING)
    {
        if (moving_piece.color == COLOR_WHITE)
        {
            gs->castling_white_kingside  = false;
            gs->castling_white_queenside = false;
        }
        else
        {
            gs->castling_black_kingside  = false;
            gs->castling_black_queenside = false;
        }
    }

    // Clear castling rights when a rook departs its starting square.
    if (moving_piece.piece == PIECE_ROOK)
    {
        if (move->from_rank == 0 && move->from_file == 0) gs->castling_white_queenside = false;
        if (move->from_rank == 0 && move->from_file == 7) gs->castling_white_kingside  = false;
        if (move->from_rank == 7 && move->from_file == 0) gs->castling_black_queenside = false;
        if (move->from_rank == 7 && move->from_file == 7) gs->castling_black_kingside  = false;
    }

    // Clear castling rights when a rook is captured on its starting square.
    if (captured.piece == PIECE_ROOK)
    {
        if (move->to_rank == 0 && move->to_file == 0) gs->castling_white_queenside = false;
        if (move->to_rank == 0 && move->to_file == 7) gs->castling_white_kingside  = false;
        if (move->to_rank == 7 && move->to_file == 0) gs->castling_black_queenside = false;
        if (move->to_rank == 7 && move->to_file == 7) gs->castling_black_kingside  = false;
    }

    // Advance turn.
    gs->side_to_move = (gs->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
}

bool IsInCheck(const Board* board, Color color)
{
    ASSERT(board);
    ASSERT(color != COLOR_NONE);

    // Locate the king for the given color.
    int8 king_rank = -1;
    int8 king_file = -1;
    for (int32 rank = 0; rank < 8; ++rank)
    {
        for (int32 file = 0; file < 8; ++file)
        {
            const Square sq = board->squares[rank][file];
            if (sq.piece == PIECE_KING && sq.color == color)
            {
                king_rank = (int8)rank;
                king_file = (int8)file;
            }
        }
    }

    // No king found — cannot be in check.
    if (king_rank == -1) return false;

    const Color enemy = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    return IsSquareAttackedBy(board, king_rank, king_file, enemy);
}

void GetLegalMoves(const GameState* gs, MoveList* out)
{
    ASSERT(gs);
    ASSERT(out);

    // Collect all pseudo-legal candidate moves for the side to move.
    MoveList candidates = {};
    GeneratePawnMoves    (gs, &candidates);
    GenerateKnightMoves  (gs, &candidates);
    GenerateRookMoves    (gs, &candidates);
    GenerateBishopMoves  (gs, &candidates);
    GenerateQueenMoves   (gs, &candidates);
    GenerateKingMoves    (gs, &candidates);
    GenerateCastlingMoves(gs, &candidates);

    const Color color = gs->side_to_move;

    // Keep only moves that do not leave the moving side's king in check.
    for (int32 i = 0; i < candidates.count; ++i)
    {
        GameState temp = *gs;
        ApplyMove(&temp, &candidates.moves[i]);
        if (!IsInCheck(&temp.board, color))
        {
            ASSERT(out->count < MAX_MOVES_PER_POSITION);
            out->moves[out->count++] = candidates.moves[i];
        }
    }
}
