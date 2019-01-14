#include <iostream>

#include "movegen.h"
#include "bitboard.h"

namespace Movegen {

	const int victimScore[13] = { 0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 };
	int mvvLVaScores[13][13];

	void init_mvvlva() {
		for (int attacker = 0; attacker <= 12; ++attacker) {
			for (int victim = 0; victim <= 12; ++victim) {
				mvvLVaScores[victim][attacker] = victimScore[victim] + 6 - (victimScore[attacker] / 100);
			}
		}
	}

	static void add_quiet_move(const Position& pos, Movelist *list, Move move) {
		list->moves[list->count].move = move;

		if (move == pos.killer_move1())
			list->moves[list->count].score = 900000;
		else if (move == pos.killer_move2())
			list->moves[list->count].score = 800000;
		else
			list->moves[list->count].score = pos.history_move(move);

		list->count++;
	}

	static void add_capture_move(const Position& pos, Movelist *list, Move move) {
		list->moves[list->count].move = move;
		list->moves[list->count].score = mvvLVaScores[pos.piece_on_sq(to_sq(move))][pos.piece_on_sq(from_sq(move))] + 1000000;
		list->count++;
	}

	static void add_en_passant_move(const Position& pos, Movelist *list, Move move) {
		list->moves[list->count].move = move;
		list->moves[list->count].score = 105 + 1000000;
		list->count++;
	}

	void add_piece_moves(Position& pos, Movelist* list, Square from) {
		Color color = pos.side_to_move();
		PieceType movingPt = pos.piece_on_sq(from);
		Bitboard moves = pos.attacks_from(movingPt, from) & (~OccupiedBB[color][ANY_PIECE]);

		while (moves) {
			Square to = pop_lsb(moves);
			PieceType capturedPt = pos.piece_on_sq(to);

			if (capturedPt) {
				add_capture_move(pos, list, make(from, to, movingPt, capturedPt, NO_PIECE, NO_FLAG));
			} else {
				add_quiet_move(pos, list, make(from, to, movingPt, NO_PIECE, NO_PIECE, NO_FLAG));
			}
		}
	}

	void add_castling_moves(Position& pos, Movelist* list) {
		if (pos.side_to_move() == WHITE) {
			if (pos.can_castle(WKCA)) {
				if ((pos.piece_on_sq(SQ_F1) == NO_PIECE) && (pos.piece_on_sq(SQ_G1) == NO_PIECE)) {
					if (!(pos.attackers_to(SQ_E1) & OccupiedBB[BLACK][ANY_PIECE])) {
						if (!(pos.attackers_to(SQ_F1) & OccupiedBB[BLACK][ANY_PIECE])) {
							add_quiet_move(pos, list, make(SQ_E1, SQ_G1, KING, NO_PIECE, NO_PIECE, FLAGCA));
						}
					}
				}
			}

			if (pos.can_castle(WQCA)) {
				if ((pos.piece_on_sq(SQ_D1) == NO_PIECE) && (pos.piece_on_sq(SQ_C1) == NO_PIECE) && (pos.piece_on_sq(SQ_B1) == NO_PIECE)) {
					if (!(pos.attackers_to(SQ_E1) & OccupiedBB[BLACK][ANY_PIECE])) {
						if (!(pos.attackers_to(SQ_D1) & OccupiedBB[BLACK][ANY_PIECE])) {
							add_quiet_move(pos, list, make(SQ_E1, SQ_C1, KING, NO_PIECE, NO_PIECE, FLAGCA));
						}
					}
				}
			}

		} else {
			if (pos.can_castle(BKCA)) {
				if ((pos.piece_on_sq(SQ_F8) == NO_PIECE) && (pos.piece_on_sq(SQ_G8) == NO_PIECE)) {
					if (!(pos.attackers_to(SQ_E8) & OccupiedBB[WHITE][ANY_PIECE])) {
						if (!(pos.attackers_to(SQ_F8) & OccupiedBB[WHITE][ANY_PIECE])) {
							add_quiet_move(pos, list, make(SQ_E8, SQ_G8, KING, NO_PIECE, NO_PIECE, FLAGCA));
						}
					}
				}
			}

			if (pos.can_castle(BQCA)) {
				if ((pos.piece_on_sq(SQ_D8) == NO_PIECE) && (pos.piece_on_sq(SQ_C8) == NO_PIECE) && (pos.piece_on_sq(SQ_B8) == NO_PIECE)) {
					if (!(pos.attackers_to(SQ_E8) & OccupiedBB[WHITE][ANY_PIECE])) {
						if (!(pos.attackers_to(SQ_D8) & OccupiedBB[WHITE][ANY_PIECE])) {
							add_quiet_move(pos, list, make(SQ_E8, SQ_C8, KING, NO_PIECE, NO_PIECE, FLAGCA));
						}
					}
				}
			}
		}
	}

	void add_pawn_moves(Position& pos, Movelist* list, Square from) {
		unsigned int rank = from >> 3;
		Color color = pos.side_to_move();
		Bitboard captures = pos.attacks_from<PAWN>(from, color) & OccupiedBB[color ^ 1][ANY_PIECE];

		if (color == WHITE) {
			Bitboard singlePushMoves = single_push_targets_white(SquareBB[from], ~OccupiedBB[BOTH][ANY_PIECE]);
			
			if (rank == RANK_7) {
				while (singlePushMoves) {
					Square to = pop_lsb(singlePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, QUEEN, NO_FLAG));
					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, ROOK, NO_FLAG));
					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, BISHOP, NO_FLAG));
					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, KNIGHT, NO_FLAG));
				}

				while (captures) {
					Square to = pop_lsb(captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, QUEEN, NO_FLAG));
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, ROOK, NO_FLAG));
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, BISHOP, NO_FLAG));
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, KNIGHT, NO_FLAG));
				}

			} else if (rank == RANK_2) {
				Bitboard doublePushMoves = double_push_targets_white(SquareBB[from], ~OccupiedBB[BOTH][ANY_PIECE]);

				while (singlePushMoves) {
					Square to = pop_lsb(singlePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, NO_PIECE, NO_FLAG));
				}

				while (doublePushMoves) {
					Square to = pop_lsb(doublePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, NO_PIECE, FLAGPS));
				}

				while (captures) {
					Square to = pop_lsb(captures);
					PieceType capturedPiece = pos.piece_on_sq(to);
					
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, NO_PIECE, NO_FLAG));
				}

			} else {
				while (singlePushMoves) {
					Square to = pop_lsb(singlePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, NO_PIECE, NO_FLAG));
				}

				while (captures) {
					Square to = pop_lsb(captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, NO_PIECE, NO_FLAG));
				}

				if (rank == RANK_5) {
					if (from + 7 == pos.en_passant()) {
						add_en_passant_move(pos, list, make(from, from + NORTH_WEST, PAWN, NO_PIECE, NO_PIECE, FLAGEP));
					} else if (from + 9 == pos.en_passant()) {
						add_en_passant_move(pos, list, make(from, from + NORTH_EAST, PAWN, NO_PIECE, NO_PIECE, FLAGEP));
					}
				}
			}
		} else {
			Bitboard singlePushMoves = single_push_targets_black(SquareBB[from], ~OccupiedBB[BOTH][ANY_PIECE]);

			if (rank == RANK_2) {
				while (singlePushMoves) {
					Square to = pop_lsb(singlePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, QUEEN, NO_FLAG));
					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, ROOK, NO_FLAG));
					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, BISHOP, NO_FLAG));
					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, KNIGHT, NO_FLAG));
				}

				while (captures) {
					Square to = pop_lsb(captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, QUEEN, NO_FLAG));
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, ROOK, NO_FLAG));
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, BISHOP, NO_FLAG));
					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, KNIGHT, NO_FLAG));
				}
			} else if (rank == RANK_7) {
				Bitboard doublePushMoves = double_push_targets_black(SquareBB[from], ~OccupiedBB[BOTH][ANY_PIECE]);

				while (singlePushMoves) {
					Square to = pop_lsb(singlePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, NO_PIECE, NO_FLAG));
				}

				while (doublePushMoves) {
					Square to = pop_lsb(doublePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, NO_PIECE, FLAGPS));
				}

				while (captures) {
					Square to = pop_lsb(captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, NO_PIECE, NO_FLAG));
				}
			}
			else {
				while (singlePushMoves) {
					Square to = pop_lsb(singlePushMoves);

					add_quiet_move(pos, list, make(from, to, PAWN, NO_PIECE, NO_PIECE, NO_FLAG));
				}

				while (captures) {
					Square to = pop_lsb(captures);
					PieceType capturedPiece = pos.piece_on_sq(to);

					add_capture_move(pos, list, make(from, to, PAWN, capturedPiece, NO_PIECE, NO_FLAG));
				}

				if (rank == RANK_4) {
					if (from + SOUTH_EAST == pos.en_passant()) {
						add_en_passant_move(pos, list, make(from, from + SOUTH_EAST, PAWN, NO_PIECE, NO_PIECE, FLAGEP));
					} else if (from + SOUTH_WEST == pos.en_passant()) {
						add_en_passant_move(pos, list, make(from, from + SOUTH_WEST, PAWN, NO_PIECE, NO_PIECE, FLAGEP));
					}
				}
			}
		}
	}

	void get_moves(Position& pos, Movelist* list) {
		Color color = pos.side_to_move();
		Bitboard pawns = OccupiedBB[color][PAWN];
		Bitboard pieces = OccupiedBB[color][ANY_PIECE] ^ pawns;

		while (pawns) {
			unsigned long sq;
			_BitScanForward64(&sq, pawns);
			clear_bit(pawns, Square(sq));

			add_pawn_moves(pos, list, (Square)sq);
		}

		while (pieces) {
			unsigned long sq;
			_BitScanForward64(&sq, pieces);
			clear_bit(pieces, Square(sq));
			
			add_piece_moves(pos, list, (Square)sq);
		}

		if (pos.castling_rights()) {
			add_castling_moves(pos, list);
		}
	}

	void add_captures(Position& pos, Movelist* list, Square from) {
		Bitboard captures = 0;
		Color color = pos.side_to_move();
		Color enemyColor = color == WHITE ? BLACK : WHITE;
		PieceType movingPt = pos.piece_on_sq(from);

		if (movingPt == PAWN) {
			captures = pos.attacks_from<PAWN>(from, color) & OccupiedBB[enemyColor][ANY_PIECE];
		} else {
			captures = pos.attacks_from(movingPt, from) & (~OccupiedBB[color][ANY_PIECE]);
			captures &= OccupiedBB[enemyColor][ANY_PIECE];
		}

		while (captures) {
			Square to = pop_lsb(captures);
			PieceType capturedPt = pos.piece_on_sq(to);

			add_capture_move(pos, list, make(from, to, movingPt, capturedPt, NO_PIECE, NO_FLAG));
		}
	}
	
	void get_captures(Position& pos, Movelist* list) {
		int color = pos.side_to_move();
		Bitboard allpieces = OccupiedBB[color][ANY_PIECE];

		while (allpieces) {
			unsigned long sq;
			_BitScanForward64(&sq, allpieces);
			clear_bit(allpieces, Square(sq));

			add_captures(pos, list, (Square)sq);
		}
	}
}