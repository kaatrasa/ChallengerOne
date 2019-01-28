#include <iostream>

#include "typeconvertions.h"
#include "defs.h"

namespace TypeConvertions {

	int to_int(char c) {
		return c - '0';
	}

    std::string to_string(char c) {
        return std::string(1, c);
    }

	Square str_to_sq(std::string sq) {
		File fileNumber = char_to_file(sq.at(0));
		Rank rankNumber = char_to_rank(sq.at(1));

		return Square(fileNumber + 8 * (rankNumber - 1));
	}
	
	std::string int_to_square(int sq) {
		int rank = sq >> 3;
		int file = sq & 7;
		std::string square;
		square.push_back(int_to_file(file));
		square.push_back(int_to_rank(rank));
		return square;
	}

	File char_to_file(char file) {
		if (file == 'a') return FILE_A;
		if (file == 'b') return FILE_B;
		if (file == 'c') return FILE_C;
		if (file == 'd') return FILE_D;
		if (file == 'e') return FILE_E;
		if (file == 'f') return FILE_F;
		if (file == 'g') return FILE_G;
		return FILE_H;
	}

	Rank char_to_rank(char rank) {
		return (Rank)(rank - '0');
	}

	char int_to_file(int file) {
		if (file == FILE_A) return 'a';
		if (file == FILE_B) return 'b';
		if (file == FILE_C) return 'c';
		if (file == FILE_D) return 'd';
		if (file == FILE_E) return 'e';
		if (file == FILE_F) return 'f';
		if (file == FILE_G) return 'g';
		return 'h';
	}

	char int_to_rank(int rank) {
		if (rank == RANK_1) return '1';
		if (rank == RANK_2) return '2';
		if (rank == RANK_3) return '3';
		if (rank == RANK_4) return '4';
		if (rank == RANK_5) return '5';
		if (rank == RANK_6) return '6';
		if (rank == RANK_7) return '7';
		return '8';
	}

    Piece char_to_piece(char charPiece) {
        if (charPiece == 'P') return wP;
        if (charPiece == 'N') return wN;
        if (charPiece == 'B') return wB;
        if (charPiece == 'R') return wR;
        if (charPiece == 'Q') return wQ;
        if (charPiece == 'K') return wK;
        if (charPiece == 'p') return bP;
        if (charPiece == 'n') return bN;
        if (charPiece == 'b') return bB;
        if (charPiece == 'r') return bR;
        if (charPiece == 'q') return bQ;
        if (charPiece == 'k') return bK;
		return EMPTY;
    }

    char piece_to_char(Piece piece) {
        if (piece == wP) return 'P';
        if (piece == wN) return 'N';
        if (piece == wB) return 'B';
        if (piece == wR) return 'R';
        if (piece == wQ) return 'Q';
        if (piece == wK) return 'K';
        if (piece == bP) return 'p';
        if (piece == bN) return 'n';
        if (piece == bB) return 'b';
        if (piece == bR) return 'r';
        if (piece == bQ) return 'q';
        if (piece == bK) return 'k';
		return '-';
    }

	std::string move_to_string(Move move) {
		char charArray[6];

		Square from = from_sq(move);
		Square to = to_sq(move);
		PieceType prom = promoted_piece(move);

		charArray[0] = int_to_file(from & 7);
		charArray[1] = int_to_rank(from >> 3);
		charArray[2] = int_to_file(to & 7);
		charArray[3] = int_to_rank(to >> 3);
		
		if (prom == NO_PIECE) {
			return std::string(charArray).substr(0, 4);
		}
		else {
			if (prom == ROOK) {
				charArray[4] = 'r';
			}
			else if (prom == BISHOP) {
				charArray[4] = 'b';
			}
			else if (prom == QUEEN) {
				charArray[4] = 'q';
			}
			else if (prom == KNIGHT) {
				charArray[4] = 'n';
			}
			return std::string(charArray).substr(0, 5);
		}
	}
}
