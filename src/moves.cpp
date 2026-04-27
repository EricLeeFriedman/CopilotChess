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
                }
            }
        }
    }
}

void ApplyMove(GameState* gs, const Move* move)
{
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

    // Advance turn.
    gs->side_to_move = (gs->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
}
