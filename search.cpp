#include <algorithm>
#include <iostream>
#include <cmath>

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

	static Value qsearch(Value alpha, Value beta, Position& pos, SearchInfo& info) {
		Timeman::check_time_up(info);
		info.nodes++;

		if (pos.is_repetition() || pos.fifty_move() >= 100) return VALUE_DRAW;
		if (pos.ply() > DEPTH_MAX - 1) return Evaluation::evaluate(pos);

		Value score = Evaluation::evaluate(pos);

		// Stand pat. Return immediately if static value is at least beta
		if (score >= beta)
			return score;

		if (score > alpha)
			alpha = score;

		Movelist list = Movelist();
		Movegen::get_captures(pos, list);

		int legal = 0;
		Value oldAlpha = alpha;
	    score = -VALUE_INFINITE;

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			pick_move(moveNum, list);
			if (!pos.do_move(list.moves[moveNum].move)) continue;

			legal++;
			score = -qsearch(-beta, -alpha, pos, info);
			pos.undo_move();

			if (info.stopped) {
				return VALUE_NONE;
			}

			if (score > alpha) {
				if (score >= beta)
					return beta;

				alpha = score;
			}
		}

		return alpha;
	}

	static Value search(Value alpha, Value beta, Depth depth, Position& pos, SearchInfo& info, bool nullOk) {
		Timeman::check_time_up(info);

		if (pos.ply() > DEPTH_MAX - ONE_PLY)
			return Evaluation::evaluate(pos);

		if (pos.is_repetition() || pos.fifty_move() >= 100)
			return VALUE_DRAW;

		// Mate distance pruning. Even if we mate at the next move our
		// alpha is already bigger because a shorter mate was found
		alpha = std::max(mated_in(pos.ply()), alpha);
		beta = std::min(mate_in(pos.ply() + 1), beta);
		if (alpha >= beta)
			return alpha;

		Value alphaOrig = alpha;
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
		if (depth == DEPTH_ZERO) return qsearch(alpha, beta, pos, info);
		info.nodes++;

		// Null move pruning
		if (nullOk 
			&& depth >= 4
			&& !inCheck
			&& pos.ply() 
			&& pos.non_pawn_material(us)) 
		{
			pos.do_null_move();
			Value score = -search(-beta, -beta + 1, depth - 4 * ONE_PLY, pos, info, false);
			pos.undo_null_move();
			
			if (score >= beta) 
				return beta;
		}

		Movelist list = Movelist();
		Movegen::get_moves(pos, list);
		Value bestScore = -VALUE_INFINITE;
		
		if (pvMove != MOVE_NONE) {
			for (int moveNum = 0; moveNum < list.count; ++moveNum) {
				if (list.moves[moveNum].move == pvMove) {
					list.moves[moveNum].score = 2000000;
					break;
				}
			}
		}

		Move bestMove = MOVE_NONE, move = MOVE_NONE;
		Value score = -VALUE_INFINITE;
		Value eval;
		int legal = 0;

		if (depth == 1) {
			eval = Evaluation::evaluate(pos);
			eval = us == WHITE ? eval : -eval;
		}

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			bool searchFullDepth = true;
			Value childScore;

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
				&& !gaveCheck
				&& !pos.advanced_pawn_push(move)
				&& eval + Value(400) <= alpha // Futility margin
				&& eval < VALUE_KNOWN_WIN) // Do not return unproven wins
			{
				pos.undo_move();
				continue;
			}

			// Late move reductions
			if (moveNum > 4 
				&& depth > 2
				&& !inCheck
				&& !isCapture 
				&& !isPromotion
				&& !gaveCheck) 
			{
				Depth reducedDepth = moveNum <= 6 ? 2 * ONE_PLY : depth / 3 + ONE_PLY;
				childScore = -search(-beta, -alpha, depth - reducedDepth, pos, info, true);
				if (childScore <= alpha) searchFullDepth = false;
			}

			if (searchFullDepth)
				childScore = -search(-beta, -alpha, depth - ONE_PLY, pos, info, true);

			pos.undo_move();

			if (info.stopped)
				return VALUE_NONE;

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
						pos.history_move_set(bestMove, (int) depth*depth);
				}
			}
		}

		if (legal == 0) {
			if (inCheck)
				return mated_in(pos.ply());
			else
				return VALUE_DRAW;
		}

		if (score <= alphaOrig)
			TT.save(pos.pos_key(), bestMove, score, UPPERBOUND, depth);
		else if (score >= beta)
			TT.save(pos.pos_key(), bestMove, score, LOWERBOUND, depth);
		else
			TT.save(pos.pos_key(), bestMove, score, EXACT, depth);

		return score;
	}

	bool found_mate(Value score, Depth& mateDepth) {

		if (score >= VALUE_MATE_IN_MAX_PLY) {
			double depth = VALUE_MATE - score;
			depth /= 2;
			mateDepth = Depth(int(ceil(depth)));
		
			return true;
		} else if (score <= VALUE_MATED_IN_MAX_PLY) {
			double depth = -VALUE_MATE - score;
			depth /= 2;
			mateDepth = Depth(int(floor(depth)));

			return true;
		}
		return false;
	}
	
	void start(Position& pos, SearchInfo& info) {
		Move bestMove = MOVE_NONE;
		Value alpha = -VALUE_INFINITE;
		Value beta = VALUE_INFINITE;
		Value eval;
		Depth currentDepth = ONE_PLY;
		Depth mateDepth;
		int pvmNum = 0;
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

			if (found_mate(eval, mateDepth))
				std::cout << "info score mate " << mateDepth;
			else
				std::cout << "info score cp " << eval;
			
			std::cout << " depth " << currentDepth
				      << " nodes " << info.nodes
				      << " time " << Timeman::get_time() - info.startTime
				      << " ";
			
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