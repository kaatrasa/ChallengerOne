#include <iostream>
#include <sstream>
#include <stdio.h>
#include <thread>
#include <memory>

#include "uci.h"
#include "tt.h"
#include "timeman.h"
#include "search.h"
#include "bitboard.h"
#include "movegen.h"
#include "utils/perft.h"
#include "utils/typeconvertions.h"

using namespace std;

void init() {
	init_bitboards();
	Movegen::init_mvvlva();
	Zobrist::init_keys();
}

Move parse_move(Position& pos, string moveStr) {
	if (moveStr[1] > '8' || moveStr[1] < '1') return MOVE_NONE;
	if (moveStr[3] > '8' || moveStr[3] < '1') return MOVE_NONE;
	if (moveStr[0] > 'h' || moveStr[0] < 'a') return MOVE_NONE;
	if (moveStr[2] > 'h' || moveStr[2] < 'a') return MOVE_NONE;

	Square from = TypeConvertions::str_to_sq(moveStr.substr(0, 2));
	Square to = TypeConvertions::str_to_sq(moveStr.substr(2, 2));

	Movelist list = Movelist();
	Movegen::get_moves(pos, list);
	int moveNum = 0;
	Move move = MOVE_NONE;
	PieceType promPce = NO_PIECE;

	for (moveNum = 0; moveNum < list.count; ++moveNum) {
		move = list.moves[moveNum].move;
		if (from_sq(move) == from && to_sq(move) == to) {
			promPce = promoted(move);
			if (promPce != NO_PIECE) {
				if (promPce == ROOK && moveStr.back() == 'r') {
					return move;
				}
				else if (promPce == BISHOP && moveStr.back() == 'b') {
					return move;
				}
				else if (promPce == QUEEN && moveStr.back() == 'q') {
					return move;
				}
				else if (promPce == KNIGHT && moveStr.back() == 'n') {
					return move;
				}
				continue;
			}
			return move;
		}
	}

	return MOVE_NONE;
}

const string StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
std::thread SearchThread;

namespace UCI{

	void uci() {
		cout << "id name ChallengerOne" << endl;
		cout << "id author VK" << endl;
		cout << "uciok" << endl;
	}

	void go_perft(Position& pos, istringstream& is) {
		string token;
		unsigned int depth;

		is >> token;
		is >> depth;

		if (token == "auto") Perft::perft_auto(pos, depth);
		else {
			try {
				depth = stoi(token);
			}
			catch (const invalid_argument exception) {
				depth = 0;
			}
			Perft::perft_begin(pos, depth, true);
		}
	}

	void go(Position& pos, SearchInfo& info, istringstream& is) {
		string token;
		int depth = -1, movestogo = 30, movetime = -1;
		int time = -1, inc = 0;
		info.timeSet = false;

		while (is >> token)
			if (token == "wtime")		   is >> time;
			else if (token == "btime")     is >> time;
			else if (token == "winc")      is >> inc;
			else if (token == "binc")      is >> inc;
			else if (token == "movestogo") is >> movestogo;
			else if (token == "depth")     is >> depth;
			else if (token == "movetime")  is >> movetime;
			else if (token == "perft")	   return go_perft(pos, is);
		
		if (movetime != -1) {
			time = movetime;
			movestogo = 1;
		}

		info.startTime = Timeman::get_time();
		info.depth = depth;

		if (time != -1) {
			info.timeSet = true;
			time /= movestogo;
			time -= 50;
			info.stopTime = info.startTime + time + inc;
		}

		if (depth == -1) {
			info.depth = MAX_DEPTH;
		}

		cout << "time:" << time << " start:" << info.startTime 
		<< " stop:" << info.stopTime << " depth:" << info.depth
		<< " timeset:" << info.timeSet << endl;
		
		SearchThread = thread(Search::start, pos, std::ref(info));
	}

	void position(Position& pos, istringstream& is) {
		Move move;
		string token, fen;
		pos.his_ply_reset();

		is >> token;
		if (token == "startpos")
		{
			fen = StartFEN;
			is >> token; // Consume "moves" token if any
		}
		else if (token == "fen")
			while (is >> token && token != "moves")
				fen += token + " ";
		else
			return;

		pos.set(fen);
		
		while (is >> token && (move = parse_move(pos, token)) != MOVE_NONE)
		{
			pos.do_move(move);
		}
		pos.ply_reset();
	}

	void ucinewgame(Position& pos, SearchInfo& info) {
		info = SearchInfo();
		pos = Position();
		info.quit = false;
		info.stopped = false;
		pos.set(StartFEN);
		TT.clear();
	}

	void stop(SearchInfo& info) {
		info.stopped = true;
		if (SearchThread.joinable()) SearchThread.join();
	}

	void loop() {
		string token, cmd;
		SearchInfo info = SearchInfo();
		Position pos = Position();

		init();
		uci();

		info.quit = false;
		info.stopped = false;
		pos.set(StartFEN);

		while (true) {
			getline(cin, cmd);
			istringstream is(cmd);

			token.clear();
			is >> skipws >> token;

			if (info.stopped && SearchThread.joinable())
				SearchThread.join();

			if (token == "quit") {
				info.quit = true;
				stop(info);
				break; 
			}
			else if (token == "stop") stop(info);
			else if (token == "uci") uci();
			else if (token == "isready") cout << "readyok" << endl;
			else if (token == "go") go(pos, info, is);
			else if (token == "position") position(pos, is);
			else if (token == "ucinewgame") ucinewgame(pos, info);
			else if (token == "print") pos.print();
		}
	}
}