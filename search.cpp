#include <algorithm>
#include <intrin.h>
#include <iostream>

#include "search.h"
#include "evaluate.h"
#include "bitboard.h"
#include "movegen.h"
#include "timeman.h"
#include "uci.h"
#include "tt.h"
#include "utils/typeconvertions.h"

namespace Search {

	static void pick_move(int moveNum, Movelist& list) {
		MoveEntry temp;
		int bestScore = 0;
		int bestNum = moveNum;

		for (int index = moveNum; index < list.count; ++index) {
			if (list.moves[index].score > bestScore) {
				bestScore = list.moves[index].score;
				bestNum = index;
			}
		}
		temp = list.moves[moveNum];
		list.moves[moveNum] = list.moves[bestNum];
		list.moves[bestNum] = temp;
	}

	static void clear_for_search(Position& pos, SearchInfo& info) {
		pos.history_moves_reset();
		pos.killer_moves_reset();
		pos.ply_reset();

		info.startTime = Timeman::get_time();
		info.stopped = 0;
		info.nodes = 0;
		info.fh = 0;
		info.fhf = 0;
	}

	static int quiescence(int alpha, int beta, Position& pos, SearchInfo& info) {
		Timeman::check_time_up(info);
		
		info.nodes++;

		if (pos.is_repetition() || pos.fifty_move() >= 100) return 0;
		if (pos.ply() > MAX_DEPTH - 1) return Evaluation::evaluate(pos);

		int score = Evaluation::evaluate(pos);

		if (score >= beta) {
			return beta;
		}

		if (score > alpha) {
			alpha = score;
		}

		Movelist list = Movelist();
		list.count = 0;
		Movegen::get_captures(pos, list);

		int legal = 0;
		int oldAlpha = alpha;
		int bestMove = MOVE_NONE;
	    score = -INF;

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {

			pick_move(moveNum, list);
			if (!pos.do_move(list.moves[moveNum].move)) continue;

			legal++;
			score = -quiescence(-beta, -alpha, pos, info);
			pos.undo_move();

			if (info.stopped) {
				return 0;
			}

			// Best move for maximizing player
			if (score > alpha) {
				// Bad move for minimizing player
				if (score >= beta) {

					// if we searched best move first
					if (legal == 1) info.fhf++;

					info.fh++;
					return beta;
				}

				alpha = score;
				bestMove = list.moves[moveNum].move;
			}
		}

		return alpha;
	}

	static int alphabeta(int alpha, int beta, int depth, Position& pos, SearchInfo& info) {

		if (depth == 0) return quiescence(alpha, beta, pos, info);

		Timeman::check_time_up(info);

		info.nodes++;

		if (pos.is_repetition() || pos.fifty_move() >= 100) return 0;
		if (pos.ply() > MAX_DEPTH - 1) return Evaluation::evaluate(pos);

		int score = -INF;
		Move pvMove = MOVE_NONE;

		if (TT.probe(pos.pos_key(), &pvMove, &score, alpha, beta, depth))
			return score;

		Movelist list = Movelist();
		list.count = 0;
		Movegen::get_moves(pos, list);

		int legal = 0;
		int oldAlpha = alpha;
		Move bestMove = MOVE_NONE;
		int bestScore = -INF;
		score = -INF;

		if (pvMove != MOVE_NONE) {

			for (int moveNum = 0; moveNum < list.count; ++moveNum) {

				if (list.moves[moveNum].move == pvMove) {
					list.moves[moveNum].score = 2000000;
					break;
				}
			}
		}

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			pick_move(moveNum, list);
			if (!pos.do_move(list.moves[moveNum].move)) continue;

			legal++;
			score = -alphabeta(-beta, -alpha, depth - 1, pos, info);
			pos.undo_move();

			if (info.stopped) {
				return 0;
			}

			if (score > bestScore) {
				bestScore = score;
				bestMove = list.moves[moveNum].move;

				// Best move for maximizing player
				if (score > alpha) {
					// Bad move for minimizing player
					if (score >= beta) {

						// if we searched best move first
						if (legal == 1) info.fhf++;

						info.fh++;

						if (!(bestMove & FLAGCAP)) {
							pos.killer_move_set(bestMove);
						}
						
						TT.save(pos.pos_key(), bestMove, beta, LOWERBOUND, depth);
						return beta;
					}

					alpha = score;

					if (!(bestMove & FLAGCAP))
						pos.history_move_set(bestMove, depth*depth);
				}
			}
		}
		
		if (legal == 0) {
			// checkmate in ply-moves
			if (pos.attackers_to(pos.king_sq()) & OccupiedBB[pos.side_to_move() ^ 1][ANY_PIECE]) {
				return -MATE_SCORE + pos.ply();
			}

			// stalemate
			else {
				return 0;
			}
		}	

		if (alpha != oldAlpha)
			TT.save(pos.pos_key(), bestMove, bestScore, EXACT, depth);
		else
			TT.save(pos.pos_key(), bestMove, alpha, UPPERBOUND, depth);

		return alpha;
	}

	void start(Position& pos, SearchInfo& info) {
		int bestScore = -INF;
		Move bestMove = MOVE_NONE;
		int pvmNum = 0;
		int currentDepth = 1;
		clear_for_search(pos, info);

		while (currentDepth <= info.depth) {

			bestScore = alphabeta(-INF, INF, currentDepth, pos, info);
			if (info.stopped) {
				break;
			}
			
			if (abs(bestScore) >= MATE_SCORE - MAX_DEPTH) {
				if (bestScore >= 0)
					bestScore = (MATE_SCORE - bestScore) / 2 + ((MATE_SCORE - bestScore) % 2 != 0);
				else
					bestScore = (-MATE_SCORE - bestScore) / 2;

				std::cout << "info score mate " << bestScore << " depth " << currentDepth << " nodes " << info.nodes << " time " << Timeman::get_time() - info.startTime << " ";
			} else
				std::cout << "info score cp " << bestScore << " depth " << currentDepth << " nodes " << info.nodes << " time " << Timeman::get_time() - info.startTime << " ";

			pos.print_pv(info, currentDepth);
			bestMove = pos.best_move();
			++currentDepth;
		}

		info.stopped = true;
		std::cout << "bestmove " << TypeConvertions::move_to_string(bestMove) << std::endl;
	}
}