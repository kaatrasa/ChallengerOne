#include <algorithm>
#include <cmath>
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

			if (score > alpha) {
				if (score >= beta)
					return beta;

				alpha = score;
			}
		}

		return alpha;
	}

	static int search(int alpha, int beta, int depth, Position& pos, SearchInfo& info, bool nullOk) {
		Timeman::check_time_up(info);

		if (pos.ply() > MAX_DEPTH - 1)
			return Evaluation::evaluate(pos);

		if (pos.is_repetition() || pos.fifty_move() >= 100)
			return 0;

		int alphaOrig = alpha;
		Move pvMove = MOVE_NONE;
		bool found;

		// Check for position in TT
		TTEntry* ttEntry = TT.probe(pos.pos_key(), found);
		if (found && ttEntry->depth >= depth) {
			pvMove = ttEntry->move;

			if (ttEntry->flag == EXACT) {
				info.nodes++;
				return ttEntry->score;
			}
			else if (ttEntry->flag == LOWERBOUND)
				alpha = std::max(alpha, ttEntry->score);
			else if (ttEntry->flag == UPPERBOUND)
				beta = std::min(beta, ttEntry->score);
		
			if (alpha >= beta) return ttEntry->score;
		}

		Color us = pos.side_to_move();
		bool inCheck = pos.attackers_to(pos.king_sq()) & OccupiedBB[~us][ANY_PIECE];

		if (inCheck) ++depth;
		if (depth == 0) return quiescence(alpha, beta, pos, info);
		info.nodes++;

		// Null move pruning
		if (nullOk 
			&& depth >= 4
			&& !inCheck
			&& pos.ply() 
			&& pos.non_pawn_material(us)) 
		{
			pos.do_null_move();
			int score = -search(-beta, -beta + 1, depth - 4, pos, info, false);
			pos.undo_null_move();
			
			if (score >= beta) 
				return beta;
		}

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

		Move bestMove = MOVE_NONE, move = MOVE_NONE;
		int score = -INF;
		int legal = 0;
		int staticEval;

		if (depth == 1)
			staticEval = abs(Evaluation::evaluate(pos));

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			bool searchFullDepth = true;
			int childScore;

			pick_move(moveNum, list);
			move = list.moves[moveNum].move;

			if (!pos.do_move(move)) 
				continue;

			bool isCapture = move & FLAGCAP;
			bool isPromotion = move & FLAGPROM;
			bool gaveCheck = pos.attackers_to(pos.king_sq()) & OccupiedBB[us][ANY_PIECE];

			legal++;
			
			// Futility pruning: frontier
			if (depth == 1
				&& !inCheck
				&& !isCapture
				&& !isPromotion
				&& !gaveCheck)
			{
				if (staticEval + 400 <= alpha
					&& staticEval + MAX_DEPTH < MATE_SCORE) // Do not return unproven wins
				{
					pos.undo_move();
					continue;
				}
			}

			// Late move reductions
			if (moveNum > 4 
				&& depth > 2
				&& !inCheck
				&& !isCapture 
				&& !isPromotion
				&& !gaveCheck) 
			{
				int reducedDepth = moveNum <= 6 ? 2 : depth / 3 + 1;
				childScore = -search(-beta, -alpha, depth - reducedDepth, pos, info, true);
				if (childScore <= alpha) searchFullDepth = false;
			}

			if (searchFullDepth)
				childScore = -search(-beta, -alpha, depth - 1, pos, info, true);

			pos.undo_move();

			if (info.stopped)
				return 0;

			// New best move
			if (childScore > score) {
				score = childScore;
				bestMove = move;

				// Good move for maximizing player
				if (score > alpha) {
					alpha = score;

					// Too good, beta cut-off
					if (alpha >= beta) {
						if (!isCapture)
							pos.killer_move_set(bestMove);
						break;
					}

					if (!isCapture)
						pos.history_move_set(bestMove, depth*depth);
				}
			}
		}

		if (legal == 0) {
			if (inCheck)
				return -MATE_SCORE + pos.ply();
			else
				return 0;
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
		Move bestMove = MOVE_NONE;
		int alpha = -INF;
		int beta = INF;
		int eval = -INF;
		int pvmNum = 0;
		int currentDepth = 1;
		const int windowSize = 5;
		int failCount = 0;
		clear_for_search(pos, info);

		while (currentDepth <= info.depth) {
			eval = search(alpha, beta, currentDepth, pos, info, false);
			if (info.stopped) break;

			// Aspiration windows
			if (eval >= beta) {
				beta += windowSize * (2 << failCount);
				++failCount;
				continue;
			} else if (eval <= alpha) {
				alpha -= windowSize * (2 << failCount);
				++failCount;
				continue;
			}

			if (abs(eval) >= MATE_SCORE - MAX_DEPTH) {
				if (eval >= 0)
					eval = (MATE_SCORE - eval) / 2 + ((MATE_SCORE - eval) % 2 != 0);
				else
					eval = (-MATE_SCORE - eval) / 2;

				std::cout << "info score mate " << eval << " depth " << currentDepth << " nodes " << info.nodes << " time " << Timeman::get_time() - info.startTime << " ";
			} else
				std::cout << "info score cp " << eval << " depth " << currentDepth << " nodes " << info.nodes << " time " << Timeman::get_time() - info.startTime << " ";

			pos.print_pv(info, currentDepth);
			bestMove = pos.best_move();
			++currentDepth;

			alpha = eval - windowSize;
			beta = eval + windowSize;
			failCount = 0;
		}

		info.stopped = true;
		std::cout << "bestmove " << TypeConvertions::move_to_string(bestMove) << std::endl;
	}
}