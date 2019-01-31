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

	bool StopSignal = false;

	static void pick_move(int moveNum, Movelist& list) {
		MoveEntry temp;
		Order bestScore = ORDER_ZERO;
		int bestNum = moveNum;

		for (int index = moveNum; index < list.count; ++index) {
			if (list.moves[index].order > bestScore) {
				bestScore = list.moves[index].order;
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

	template <NodeType NT>
	static Value qsearch(Value alpha, Value beta, Position& pos, SearchInfo& info) {
		Timeman::check_time_up(info);
		info.nodes++;

		if (pos.is_repetition() || pos.fifty_move() >= 100) return VALUE_DRAW;
		if (pos.ply() > DEPTH_MAX - 1) return Evaluation::evaluate(pos);

		bool found;
		
		// Check for position in TT
		TTEntry* ttEntry = TT.probe(pos.pos_key(), found);
		if (found) {
			if (ttEntry->bound == BOUND_EXACT) {
				return ttEntry->value;
			}
			else if (ttEntry->bound == BOUND_LOWER)
				alpha = std::max(alpha, ttEntry->value);
			else if (ttEntry->bound == BOUND_UPPER)
				beta = std::min(beta, ttEntry->value);

			if (alpha >= beta) return ttEntry->value;
		}

		Value score = Evaluation::evaluate(pos);

		// Stand pat. Return immediately if static value is at least beta
		if (score >= beta)
			return beta;

		if (score > alpha)
			alpha = score;

		Movelist list = Movelist();
		Movegen::get_moves_noisy(pos, list);

		int legal = 0;
		Value oldAlpha = alpha;
	    score = -VALUE_INFINITE;

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			pick_move(moveNum, list);
			if (!pos.do_move(list.moves[moveNum].move)) continue;

			legal++;
			score = -qsearch<NT>(-beta, -alpha, pos, info);
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

	template<NodeType NT>
	static Value search(Value alpha, Value beta, Depth depth, Position& pos, SearchInfo& info, bool nullOk) {
		constexpr bool pvNode = NT == PV;
		const bool rootNode = pvNode && pos.ply() == 0;
		
		Color us = pos.side_to_move();
		TTEntry* ttEntry;
		Value eval, ttValue = VALUE_NONE, bestValue = -VALUE_INFINITE, childValue, alphaOrig = alpha;
		Value futilityMargin, seeMargin[2];
		Move ttMove = MOVE_NONE, bestMove = MOVE_NONE, move = MOVE_NONE;
		Depth R;
		bool ttHit, inCheck, isQuiet, doFullSearch;
		int legalCount = 0;

		// Step 1. Quiescence Search.
		if (depth <= DEPTH_ZERO) 
			return qsearch<NT>(alpha, beta, pos, info);

		assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
		assert(pvNode || (alpha == beta - 1));
		assert(DEPTH_ZERO < depth && depth < DEPTH_MAX);
		assert(depth / ONE_PLY * ONE_PLY == depth);
		info.nodes++;

		// Step 2. Time up check.
		Timeman::check_time_up(info);

		// Step 3. Check for early exit conditions.
		if (!rootNode) 
		{
			if (pos.is_repetition() || pos.fifty_move() >= 100)
				return VALUE_DRAW;

			if (pos.ply() >= DEPTH_MAX)
				return Evaluation::evaluate(pos);

			// Mate distance pruning. Even if we mate at the next move our
			// alpha is already bigger because a shorter mate was found.
			alpha = std::max(mated_in(pos.ply()), alpha);
			beta = std::min(mate_in(pos.ply() + 1), beta);
			if (alpha >= beta)
				return alpha;
		}

		// Step 4. Check for position in the transposition table.
		ttEntry = TT.probe(pos.pos_key(), ttHit);
		if (ttHit && ttEntry->depth >= depth) 
		{
			ttMove = ttEntry->move;
			ttValue = ttEntry->value;
			
			if (rootNode)
				info.bestMove = ttMove;

			if (ttEntry->bound == BOUND_EXACT) 
				return ttValue;
			else if (ttEntry->bound == BOUND_LOWER)
				alpha = std::max(alpha, ttValue);
			else if (ttEntry->bound == BOUND_UPPER)
				beta = std::min(beta, ttValue);
		
			if (alpha >= beta) return ttValue;
		}

		// Step 5. Initialize some flags and values.
		
		// Calculate whether we are in check
		inCheck = pos.attackers_to(pos.king_sq()) & pos.pieces(~us);
		
		// Calculate static evalation, reuse TT entry value if possible
		eval = ttHit && ttValue != VALUE_NONE ? ttValue
											  : Evaluation::evaluate(pos);

		// Futility Pruning Margin
		futilityMargin = eval + FutilityMargin * depth;

		// Step 6. Razoring. If a Quiescence Search for the current position
		// still falls way below alpha, we will assume that the score from
		// the Quiescence search was sufficient.
		if (   !pvNode
			&& !inCheck
			&&  depth <= RazorDepth
			&&  eval + RazorMargin < alpha)
			return qsearch<NonPV>(alpha, beta, pos, info);

		// Step 7. Reverse Futility Pruning, special case of null move pruning.
		// Our position is so good that we can take a big hit and still get 
		// the beta cutoff, prune.
		if (   !pvNode
			&& !inCheck
			&&  depth <= BetaPruningDepth
			&&  eval - BetaMargin * depth > beta)
			return eval;

		// Step 8. Null move pruning.
		// Our position is so good that we can pass the move and still get 
		// the beta cutoff, prune. Need to be bit careful of Zugzwang.
		// We avoid NMP when we have
		// information from the Transposition Table which suggests it will fail
		if (   !pvNode
			&&  nullOk 
			&&  depth >= NullMovePruningDepth
			&& !inCheck
			&&  pos.ply()
			&&  eval >= beta
			&&  pos.non_pawn_material(us))
		{

			R = Depth(4) + depth / 6 + Depth(std::min(3, int(eval - beta) / 200));

			pos.do_null_move();
			Value nullValue = -search<NonPV>(-beta, -beta + 1, depth - R, pos, info, false);
			pos.undo_null_move();
			
			if (nullValue >= beta)
			{
				// Do not return unproven mate scores
				if (nullValue >= VALUE_MATE_IN_MAX_PLY)
					nullValue = beta;

				return nullValue;
			}
		}

		if (inCheck) ++depth;

		Movelist list = Movelist();
		Movegen::get_moves(pos, list);
		
		if (ttMove != MOVE_NONE) {
			for (int moveNum = 0; moveNum < list.count; ++moveNum) {
				if (list.moves[moveNum].move == ttMove) {
					list.moves[moveNum].order = ORDER_TT;
					break;
				}
			}
		}

		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			doFullSearch = true;

			pick_move(moveNum, list);
			move = list.moves[moveNum].move;

			if (!pos.do_move(move)) 
				continue;

			legalCount++;
			isQuiet = !(move & FLAG_NOISY);

			// Futility pruning: frontier
			if (    depth == 1
				&&  isQuiet
				&& !inCheck
				&& !pos.advanced_pawn_push(move)
				&& eval + Value(600) <= alpha // Futility margin
				&& eval < VALUE_KNOWN_WIN) // Do not return unproven wins
			{
				pos.undo_move();
				continue;
			}

			// Late move reductions
			if (   legalCount > 1 
				&& depth > 2
				&& isQuiet
				&& !inCheck) 
			{
				Depth reducedDepth = moveNum <= 6 ? 2 * ONE_PLY : depth / 3 + ONE_PLY;
				childValue = -search<NonPV>(-alpha-1, -alpha, depth - reducedDepth, pos, info, true);
				if (childValue <= alpha) doFullSearch = false;
			}

			if (doFullSearch)
				childValue = -search<NT>(-beta, -alpha, depth - ONE_PLY, pos, info, true);

			pos.undo_move();

			if (info.stopped)
				return VALUE_NONE;

			// New best move
			if (childValue > bestValue) {
				bestValue = childValue;
				bestMove = move;

				// Good move for maximizing player
				if (bestValue > alpha) {
					alpha = bestValue;

					// Too good, beta cut-off
					if (alpha >= beta) {
						if (isQuiet)
							pos.killer_move_set(bestMove);
						break;
					}

					if (isQuiet)
						pos.history_move_set(bestMove, (Order)depth);
				}
			}
		}

		// Save bestmove if inside root node
		if (rootNode)
			info.bestMove = bestMove;
		
		if (legalCount == 0) return inCheck ? mated_in(pos.ply()) : VALUE_DRAW;


		// Save search results into tt
		Bound bound = bestValue >= beta ? BOUND_LOWER
					 : bestValue > alphaOrig ? BOUND_EXACT 
				     : BOUND_UPPER;

		TT.save(pos.pos_key(), bestMove, bestValue, bound, depth);

		return bestValue;
	}

	Value aspiration_window(Position& pos, SearchInfo& info, Depth depth, Value previous) {
		Value alpha, beta, value; 
		int delta = WindowSize;

		// Create an aspiration window, unless still below the starting depth
		alpha = depth >= WindowDepth ? std::max(-VALUE_INFINITE, previous - delta) : -VALUE_INFINITE;
		beta = depth >= WindowDepth ? std::min(VALUE_INFINITE, previous + delta) : VALUE_INFINITE;

		// Keep trying larger windows until one works
		while (!info.stopped) 
		{
			// Perform a search on the window, return if inside the window
			value = search<PV>(alpha, beta, depth, pos, info, false);
			if (value > alpha && value < beta)
				return value;

			// Search failed low
			if (value <= alpha) {
				beta = (alpha + beta) / 2;
				alpha = std::max(-VALUE_INFINITE, alpha - delta);
			}

			// Search failed high
			if (value >= beta)
				beta = std::min(VALUE_INFINITE, beta + delta);

			// Expand the search window
			delta = delta + delta / 2;
		}

		return VALUE_NONE;
	}
	
	void start(Position& pos, SearchInfo& info) {
		Value eval = VALUE_ZERO;
		Depth depth = ONE_PLY;

		// Prepare for search
		clear_for_search(pos, info);

		// Iterative deepening
		while (depth <= info.depth) {
	
			// Begin searching.
			eval = aspiration_window(pos, info, depth, eval);

			// Check if we were interrupted.
			if (info.stopped) break;

			// Save best move
			pos.best_move_set(info.bestMove);

			// Report results.
			UCI::report(pos, info, depth, eval);

			// Next depth
			++depth;
		}

		// Inform interface that we have stopped searching.
		// Report best move.
		info.stopped = true;
		UCI::report_best_move(pos, info);
	}
}