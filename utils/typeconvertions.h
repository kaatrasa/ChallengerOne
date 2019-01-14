#pragma once

#include <string>

#include "defs.h"

namespace TypeConvertions {
	int to_int(char c);
    std::string to_string(char c);
	std::string int_to_square(int sq);
	char int_to_file(int file);
	char int_to_rank(int rank);
    Piece char_to_piece(char charPiece);
    char piece_to_char(Piece piece);
	std::string move_to_string(Move move);
	File char_to_file(char file);
	Rank char_to_rank(char rank);
	Square str_to_sq(std::string sq);
}
