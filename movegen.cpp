#include <iostream>

#include "movegen.h"
#include "bitboard.h"

namespace Movegen {

	const Order VictimScore[13] = {ORDER_ZERO , Order(100), Order(200), Order(300), Order(400),
								   Order(500), Order(600), Order(100), Order(200), Order(300),
								   Order(400), Order(500), Order(600) };
	Order MvvLVaScores[13][13];

	void init_mvvlva() {
		for (int attacker = 0; attacker <= 12; ++attacker) {
			for (int victim = 0; victim <= 12; ++victim) {
				MvvLVaScores[victim][attacker] = VictimScore[victim] + Order(6) - (VictimScore[attacker] / 100);
			}
		}
	}

	static void add_quiet(const Position& pos, Movelist& list, Move move) {
		list.moves[list.count].move = move;

		if (move == pos.killer_move1())
			list.moves[list.count].order = ORDER_KILLER1;
		else if (move == pos.killer_move2())
			list.moves[list.count].order = ORDER_KILLER2;
		else
			list.moves[list.count].order = pos.history_move(move);

		list.count++;
	}

	static void add_capture(const Position& pos, Movelist& list, Move move) {
		list.moves[list.count].move = move;
		list.moves[list.count].order = MvvLVaScores[captured_piece(move)][moved_piece(move)] + ORDER_CAPTURE;
		list.count++;
	}

	static void add_ep(const Position& pos, Movelist& list, Move move) {
		list.moves[list.count].move = move;
		list.moves[list.count].order = ORDER_EP;
		list.count++;
	}

	static void add_promotion(const Position& pos, Movelist& list, Move move, PieceType prom) {
		assert(prom != PAWN);
		assert(prom != KING);

		list.moves[list.count].move = move;
		
		switch (prom)
		{
		case QUEEN: list.moves[list.count].order = ORDER_PROM_Q;
		case KNIGHT: list.moves[list.count].order = ORDER_PROM_N;
		case BISHOP: list.moves[list.count].order = ORDER_PROM_B;
		case ROOK: list.moves[list.count].order = ORDER_PROM_R;
		}
		list.count++;
	}

	void add_castling_moves(Position& pos, Movelist& list) {
		if (pos.side_to_move() == WHITE) {
			if (pos.can_castle(WKCA)) {
				if ((pos.piece_on_sq(SQ_F1) == PIECETYPE_NONE) && (pos.piece_on_sq(SQ_G1) == PIECETYPE_NONE)) {
					if (!(pos.attackers_to(SQ_E1) & OccupiedBB[BLACK][PIECETYPE_ANY])) {
						if (!(pos.attackers_to(SQ_F1) & OccupiedBB[BLACK][PIECETYPE_ANY])) {
							add_quiet(pos, list, make(SQ_E1, SQ_G1, KING, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_CASTLE));
						}
					}
				}
			}

			if (pos.can_castle(WQCA)) {
				if ((pos.piece_on_sq(SQ_D1) == PIECETYPE_NONE) && (pos.piece_on_sq(SQ_C1) == PIECETYPE_NONE) && (pos.piece_on_sq(SQ_B1) == PIECETYPE_NONE)) {
					if (!(pos.attackers_to(SQ_E1) & OccupiedBB[BLACK][PIECETYPE_ANY])) {
						if (!(pos.attackers_to(SQ_D1) & OccupiedBB[BLACK][PIECETYPE_ANY])) {
							add_quiet(pos, list, make(SQ_E1, SQ_C1, KING, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_CASTLE));
						}
					}
				}
			}

		} else {
			if (pos.can_castle(BKCA)) {
				if ((pos.piece_on_sq(SQ_F8) == PIECETYPE_NONE) && (pos.piece_on_sq(SQ_G8) == PIECETYPE_NONE)) {
					if (!(pos.attackers_to(SQ_E8) & OccupiedBB[WHITE][PIECETYPE_ANY])) {
						if (!(pos.attackers_to(SQ_F8) & OccupiedBB[WHITE][PIECETYPE_ANY])) {
							add_quiet(pos, list, make(SQ_E8, SQ_G8, KING, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_CASTLE));
						}
					}
				}
			}

			if (pos.can_castle(BQCA)) {
				if ((pos.piece_on_sq(SQ_D8) == PIECETYPE_NONE) && (pos.piece_on_sq(SQ_C8) == PIECETYPE_NONE) && (pos.piece_on_sq(SQ_B8) == PIECETYPE_NONE)) {
					if (!(pos.attackers_to(SQ_E8) & OccupiedBB[WHITE][PIECETYPE_ANY])) {
						if (!(pos.attackers_to(SQ_D8) & OccupiedBB[WHITE][PIECETYPE_ANY])) {
							add_quiet(pos, list, make(SQ_E8, SQ_C8, KING, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_CASTLE));
						}
					}
				}
			}
		}
	}

	// Noisy pawn moves: captures, promotions, en passant
	void add_pawn_moves_noisy(Position& pos, Movelist& list, Square from) {
		Rank rank = rank_of(from);
		Color us = pos.side_to_move();
		Bitboard captures = pos.attacks_from<PAWN>(from, us) & OccupiedBB[~us][PIECETYPE_ANY];

		if (us == WHITE) {

			if (rank == RANK_7) {
				Bitboard singlePushMoves = single_push_targets_white(SquareBB[from], ~OccupiedBB[BOTH][PIECETYPE_ANY]);

				while (singlePushMoves) {
					Square to = pop_lsb(&singlePushMoves);

					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, QUEEN, FLAG_NONE), QUEEN);
					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, ROOK, FLAG_NONE), ROOK);
					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, BISHOP, FLAG_NONE), BISHOP);
					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, KNIGHT, FLAG_NONE), KNIGHT);
				}

				while (captures) 
				{
					Square to = pop_lsb(&captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, QUEEN, FLAG_NONE), QUEEN);
					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, ROOK, FLAG_NONE), ROOK);
					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, BISHOP, FLAG_NONE), BISHOP);
					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, KNIGHT, FLAG_NONE), KNIGHT);
				}
			}
			else
			{
				while (captures) 
				{
					Square to = pop_lsb(&captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture(pos, list, make(from, to, PAWN, capturedPiece, PIECETYPE_NONE, FLAG_NONE));
				}

				if (rank == RANK_5) 
				{
					if (from + NORTH_WEST == pos.en_passant())
						add_ep(pos, list, make(from, from + NORTH_WEST, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_EP));

					else if (from + NORTH_EAST == pos.en_passant())
						add_ep(pos, list, make(from, from + NORTH_EAST, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_EP));
				}
			}
		}
		else // Black moves
		{

			if (rank == RANK_2) 
			{
				Bitboard singlePushMoves = single_push_targets_black(SquareBB[from], ~OccupiedBB[BOTH][PIECETYPE_ANY]);

				while (singlePushMoves) {
					Square to = pop_lsb(&singlePushMoves);

					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, QUEEN, FLAG_NONE), QUEEN);
					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, ROOK, FLAG_NONE), ROOK);
					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, BISHOP, FLAG_NONE), BISHOP);
					add_promotion(pos, list, make(from, to, PAWN, PIECETYPE_NONE, KNIGHT, FLAG_NONE), KNIGHT);
				}

				while (captures) {
					Square to = pop_lsb(&captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, QUEEN, FLAG_NONE), QUEEN);
					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, ROOK, FLAG_NONE), ROOK);
					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, BISHOP, FLAG_NONE), BISHOP);
					add_promotion(pos, list, make(from, to, PAWN, capturedPiece, KNIGHT, FLAG_NONE), KNIGHT);
				}

			}
			else 
			{
				while (captures) 
				{
					Square to = pop_lsb(&captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture(pos, list, make(from, to, PAWN, capturedPiece, PIECETYPE_NONE, FLAG_NONE));
				}

				if (rank == RANK_4) 
				{
					if (from + SOUTH_EAST == pos.en_passant())
						add_ep(pos, list, make(from, from + SOUTH_EAST, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_EP));

					else if (from + SOUTH_WEST == pos.en_passant())
						add_ep(pos, list, make(from, from + SOUTH_WEST, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_EP));
				}
			}
		}
	}

	// Quiet pawn moves: pawn pushes (1 or 2 steps)
	void add_pawn_moves_quiet(Position& pos, Movelist& list, Square from) {
		Rank rank = rank_of(from);
		Color us = pos.side_to_move();

		// All promotion moves are considered noisy
		if (   us == WHITE && rank == RANK_7 
			|| us == BLACK && rank == RANK_2) return;

		if (us == WHITE) {
			Bitboard singlePushMoves = single_push_targets_white(SquareBB[from], ~OccupiedBB[BOTH][PIECETYPE_ANY]);

			while (singlePushMoves) {
				Square to = pop_lsb(&singlePushMoves);

				add_quiet(pos, list, make(from, to, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_NONE));
			}

			if (rank == RANK_2) 
			{
				Bitboard doublePushMoves = double_push_targets_white(SquareBB[from], ~OccupiedBB[BOTH][PIECETYPE_ANY]);

				while (doublePushMoves) 
				{
					Square to = pop_lsb(&doublePushMoves);
					
					add_quiet(pos, list, make(from, to, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_PS));
				}
			}
		}
		else // Black moves
		{
			Bitboard singlePushMoves = single_push_targets_black(SquareBB[from], ~OccupiedBB[BOTH][PIECETYPE_ANY]);

			while (singlePushMoves) 
			{
				Square to = pop_lsb(&singlePushMoves);

				add_quiet(pos, list, make(from, to, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_NONE));
			}

			if (rank == RANK_7) 
			{
				Bitboard doublePushMoves = double_push_targets_black(SquareBB[from], ~OccupiedBB[BOTH][PIECETYPE_ANY]);
				
				while (doublePushMoves) 
				{
					Square to = pop_lsb(&doublePushMoves);

					add_quiet(pos, list, make(from, to, PAWN, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_PS));
				}
			}
		}
	}

	// Piece moves that capture something
	void add_piece_moves_noisy(Position& pos, Movelist& list, Square from, Bitboard captures) {
		PieceType movingPt = pos.piece_on_sq(from);

		assert(is_ok(movingPt));
		assert(movingPt != PAWN);

		while (captures) 
		{
			Square to = pop_lsb(&captures);
			PieceType capturedPt = pos.piece_on_sq(to);

			assert(is_ok(capturedPt));

			add_capture(pos, list, make(from, to, movingPt, capturedPt, PIECETYPE_NONE, FLAG_NONE));
		}
	}

	// Piece moves that don't capture anything
	void add_piece_moves_quiet(Position& pos, Movelist& list, Square from, Bitboard quiets) {
		PieceType movingPt = pos.piece_on_sq(from);

		while (quiets) 
		{
			Square to = pop_lsb(&quiets);

			assert(pos.piece_on_sq(to) == PIECETYPE_NONE);

			add_quiet(pos, list, make(from, to, movingPt, PIECETYPE_NONE, PIECETYPE_NONE, FLAG_NONE));
		}
	}

	void get_moves(Position& pos, Movelist& list) {
		Color us = pos.side_to_move();
		Color them = ~us;
		Bitboard pawns = OccupiedBB[us][PAWN];
		Bitboard pieces = OccupiedBB[us][PIECETYPE_ANY] ^ pawns;

		while (pawns)
		{
			Square from = pop_lsb(&pawns);

			add_pawn_moves_noisy(pos, list, from);
			add_pawn_moves_quiet(pos, list, from);
		}

		while (pieces) 
		{
			Square from = pop_lsb(&pieces);
			PieceType movingPt = pos.piece_on_sq(from);
			Bitboard moves = pos.attacks_from(movingPt, from) & (~OccupiedBB[us][PIECETYPE_ANY]);
			Bitboard captures = moves & OccupiedBB[them][PIECETYPE_ANY];

			add_piece_moves_noisy(pos, list, from, captures);
			add_piece_moves_quiet(pos, list, from, moves ^ captures);
		}

		if (pos.castling_rights())
			add_castling_moves(pos, list);
	}
	
	void get_moves_noisy(Position& pos, Movelist& list) {
		Color us = pos.side_to_move();
		Color them = ~us;
		Bitboard pawns = OccupiedBB[us][PAWN];
		Bitboard pieces = OccupiedBB[us][PIECETYPE_ANY] ^ pawns;

		while (pawns)
			add_pawn_moves_noisy(pos, list, pop_lsb(&pawns));

		while (pieces)
		{
			Square from = pop_lsb(&pieces);
			PieceType movingPt = pos.piece_on_sq(from);
			Bitboard captures = pos.attacks_from(movingPt, from) & (~OccupiedBB[us][PIECETYPE_ANY]);
			captures &= OccupiedBB[them][PIECETYPE_ANY];

			add_piece_moves_noisy(pos, list, from, captures);
		}
	}
}