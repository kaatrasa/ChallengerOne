#include <ctime>
#include <iostream>

#include "perft.h"
#include "typeconvertions.h"
#include "../movegen.h"

namespace Perft {

	void perft(Position& pos, int depth, unsigned long long& leafNodes) {
		if (depth == 0) {
			leafNodes++;
			return;
		}

		Movelist list = Movelist();
		list.count = 0;
		Movegen::get_moves(pos, list);

		int moveNum = 0;
		for (moveNum = 0; moveNum < list.count; ++moveNum) {
			Move move = list.moves[moveNum].move;
			
			if (!pos.do_move(move))
				continue;
			
			perft(pos, depth - 1, leafNodes);
			
			pos.undo_move();
		}
	}

	unsigned long long perft_begin(Position& pos, int depth, bool print) {
		std::clock_t start = std::clock();
		unsigned long long leafNodes = 0;
		Movelist list = Movelist();
		depth = depth <= 0 ? 1 : depth;

		list.count = 0;
		Movegen::get_moves(pos, list);

		Move move;
		for (int moveNum = 0; moveNum < list.count; ++moveNum) {
			move = list.moves[moveNum].move;

			if (!pos.do_move(move))
				continue;

			unsigned long long cumnodes = leafNodes;
			perft(pos, depth - 1, leafNodes);
			pos.undo_move();
			unsigned long long oldnodes = leafNodes - cumnodes;
			
			if (print) {
				std::cout << moveNum + 1 << ": " << TypeConvertions::move_to_string(move) << " nodes: " << oldnodes << std::endl;
			}
		}

		if (print) {
			std::cout << "Test Complete: " << leafNodes << " leafnodes visited" << std::endl;
			std::cout << "Time: " << (std::clock() - start) / (double)CLOCKS_PER_SEC << "seconds." << std::endl;
		}

		return leafNodes;
	}

	void perft_auto(Position& pos, int maxDepth) {
		maxDepth = maxDepth <= 0 ? 6 : maxDepth;
		std::string fens[2] = { 
			"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" ,
			"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1" 
		};
		unsigned long long expected_nodes[2][6] = {
			{ 48, 2039, 97862, 4085603, 193690690, 8031647685 },
			{ 24, 496, 9483, 182838, 3605103, 71179139 }
		};

		for (auto j = 0; j < 2; ++j) {
			for (int i = 0; i < 6 && i < maxDepth; ++i) {
				int depth = i + 1;
				
				pos.set(fens[j]);

				unsigned long long nodes = perft_begin(pos, depth, false);

				if (nodes == expected_nodes[j][i]) {
					std::cout << "OK. ";
				}
				else {
					std::cout << "FAIL. ";
				}
				std::cout << "Position: " << j  << ", depth: " << depth << ", nodes: " << nodes << std::endl;
			}
		}
	}
}












