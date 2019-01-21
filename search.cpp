#include <algorithm>
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

		if (score >= beta)
			return beta;

		if (score > alpha)
			alpha = score;

		Movelist list = Movelist();
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

	static int search(int alpha, int beta, int depth, Position& pos, SearchInfo& info) {
		Timeman::check_time_up(info);

		if (pos.ply() > MAX_DEPTH - 1)
			return Evaluation::evaluate(pos);

		if (pos.is_repetition() || pos.fifty_move() >= 100)
			return 0;

		int alphaOrig = alpha;
		Move pvMove = MOVE_NONE;
		bool found;

		TTEntry* ttEntry = TT.probe(pos.pos_key(), found);
		if (found && ttEntry->depth >= depth) {
			pvMove = ttEntry->move;

			if (ttEntry->flag == EXACT)
				return ttEntry->score;
			else if (ttEntry->flag == LOWERBOUND)
				alpha = std::max(alpha, ttEntry->score);
			else if (ttEntry->flag == UPPERBOUND)
				beta = std::min(beta, ttEntry->score);
		
			if (alpha >= beta) return ttEntry->score;
		}

		if (depth == 0)
			return quiescence(alpha, beta, pos, info);

		info.nodes++;

		Movelist list = Movelist();
		Movegen::get_moves(pos, list);
		int bestScore = -INF;
		
		if (pvMove != MOVE_NONE) {
			for (int moveNum = 0; moveNum < list.count; ++moveNum) {
				if (list.moves[moveNum].move == pvMove) {
					list.moves[moveNum].score = 2000000;
					break;
				}
			}
		}

		Move bestMove = MOVE_NONE;
		int score = -INF;
		int legal = 0;

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			// Move ordering
			pick_move(moveNum, list);
			
			// Try to move
			if (!pos.do_move(list.moves[moveNum].move)) continue;
			
			// Move ok
			legal++;

			// Evaluate the move
			int childScore = -search(-beta, -alpha, depth - 1, pos, info);
			pos.undo_move();

			if (info.stopped)
				return 0;

			// New best move
			if (childScore > score) {
				score = childScore;
				bestMove = list.moves[moveNum].move;

				// Alpha cut-off
				if (score > alpha) {
					alpha = score;

					// Beta cut-off
					if (alpha >= beta) {

						// Stats..
						if (legal == 1) info.fhf++;
						info.fh++;

						if (!(bestMove & FLAGCAP))
							pos.killer_move_set(bestMove);

						break;
					}

					if (!(bestMove & FLAGCAP))
						pos.history_move_set(bestMove, depth*depth);
				}
			}
		}

		if (legal == 0) {
			if (pos.attackers_to(pos.king_sq()) & OccupiedBB[pos.side_to_move() ^ 1][ANY_PIECE])
				return -MATE_SCORE + pos.ply(); // checkmate in ply moves
			else
				return 0; // stalemate
		}

		if (score <= alphaOrig)
			TT.save(pos.pos_key(), bestMove, score, UPPERBOUND, depth);
		else if (score >= beta)
			TT.save(pos.pos_key(), bestMove, score, LOWERBOUND, depth);
		else
			TT.save(pos.pos_key(), bestMove, score, EXACT, depth);

		return score;
	}
	
	void start(Position& pos, SearchInfo& info) {
		int bestScore = -INF;
		Move bestMove = MOVE_NONE;
		int pvmNum = 0;
		int currentDepth = 1;
		clear_for_search(pos, info);

		while (currentDepth <= info.depth) {
			bestScore = search(-INF, INF, currentDepth, pos, info);
			
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

			if (info.stopped) break;
		}

		info.stopped = true;
		std::cout << "bestmove " << TypeConvertions::move_to_string(bestMove) << std::endl;
	}
}