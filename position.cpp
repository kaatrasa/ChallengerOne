#include <iostream>
#include <random>

#include "position.h"
#include "movegen.h"
#include "tt.h"
#include "utils/typeconvertions.h"
#include "utils/stringoperators.h"

using namespace std;

namespace Zobrist {
	std::random_device rd;
	std::mt19937_64 engine64(rd());
	std::uniform_int_distribution<Key> dist64(0, UINT64_MAX);

	Key psq[2][7][64]; // color, piecetype, square
	Key enpassant[8]; // for each file
	Key castling[16];
	Key side;

	static Key rand64() {
		return dist64(engine64);
	}

	void init_keys() {
		for (Color c = WHITE; c < BOTH; ++c) {
			for (PieceType p = NO_PIECE; p <= KING; ++p) {
				for (Square s = SQ_A1; s <= SQ_H8; ++s) {
					psq[c][p][s] = rand64();
				}
			}
		}

		for (File file = FILE_A; file <= FILE_H; ++file) {
			enpassant[file] = rand64();
		}

		for (int i = 0; i < 16; ++i) {
			castling[i] = rand64();
		}

		side = rand64();
	}
}

const int CastlePerm[64] = {
	13, 15, 15, 15, 12, 15, 15, 14,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	7,  15, 15, 15,  3, 15, 15, 11
};

Position::Position() {

}

Bitboard Position::attackers_to(Square s, Bitboard occ) const {
	return (attacks_from<PAWN>(s, BLACK) & OccupiedBB[WHITE][PAWN])
		| (attacks_from<PAWN>(s, WHITE)  & OccupiedBB[BLACK][PAWN])
		| (attacks_from<KNIGHT>(s)       & OccupiedBB[BOTH][KNIGHT])
		| (attacks_bb<  ROOK>(s, occ)	 & (OccupiedBB[BOTH][ROOK] | OccupiedBB[BOTH][QUEEN]))
		| (attacks_bb<BISHOP>(s, occ)	 & (OccupiedBB[BOTH][BISHOP] | OccupiedBB[BOTH][QUEEN]))
		| (attacks_from<KING>(s)         & OccupiedBB[BOTH][KING]);
}

void Position::set(string fen) {
	fen_ = fen;
	clear_pieces();

	size_t pos = 0;
	for (int rank = RANK_8; rank >= RANK_1; --rank) {
		pos = fen.find("/");
		string fenRow = fen.substr(0, pos).substr(0, fen.find(" "));
		fen = fen.substr(pos + 1);

		int file = FILE_A;
		for (char& c : fenRow) {
			if (isdigit(c))
				file += TypeConvertions::to_int(c);
			else {
				int squareNum = rank * 8 + file;
				add_piece(squareNum, TypeConvertions::char_to_piece(c));

				++file;
			}
		}
	}

	fen = fen_;
	string fenDetails = fen.substr(fen.find(" "));
	stringoperators::trim(fenDetails);
	vector<string> details = stringoperators::split(fenDetails, ' ');

	sideToMove_ =  details.at(0) == "w" ? WHITE : BLACK;
	castlingRights_ = details.at(1).find("K") != std::string::npos ? WKCA : NO_CASTLING;
	castlingRights_ += details.at(1).find("Q") != std::string::npos ? WQCA : NO_CASTLING;
	castlingRights_ += details.at(1).find("k") != std::string::npos ? BKCA : NO_CASTLING;
	castlingRights_ += details.at(1).find("q") != std::string::npos ? BQCA : NO_CASTLING;
	enPassant_ = details.at(2) == "-" ? SQ_NONE : TypeConvertions::str_to_sq(details.at(2));
	calculate_pos_key();
}

Move Position::best_move() {
	Move move;
	int score;

	if (TT.probe(posKey_, &move, &score, -INF, INF, 0))
		return move;

	return MOVE_NONE;
}

int Position::pv(const int depth) {
	Move move = best_move();
	int count = 0;

	while (move != MOVE_NONE && count < depth) {
		if (is_real_move(move)) {
			do_move(move);
			pvArray_[count++] = move;
		} else
			break; // False move

		move = best_move();
	}

	while (ply_ > 0) {
		undo_move();
	}

	return count;
}

void Position::print_pv(S_SEARCHINFO *info, int depth) {
	int pvCount = pv(depth);
	int pvIndex = 0;
	std::cout << "pv ";
	while (pvCount) {
		Move pvMove = pvArray_[pvIndex];
		std::cout << TypeConvertions::move_to_string(pvMove) << " ";
		--pvCount;
		++pvIndex;
	}
	std::cout << std::endl;
}

bool Position::is_real_move(Move move) {
	Movelist list[1];
	list->count = 0;
	Movegen::get_moves(*this, list);

	for (int moveNum = 0; moveNum < list->count; ++moveNum) {

		if (list->moves[moveNum].move == move) {

			if (!do_move(move)) return false;
			undo_move();

			return true;
		}
	}
	return false;
}

bool Position::is_repetition() {
	int index = 0;

	for (index = hisPly_ - fiftyMove_; index < hisPly_ - 1; ++index) {
		if (posKey_ == history_[index].posKey) {
			return true;
		}
	}
	return false;
}

const string Position::fen() const {
	return fen_;
}

void Position::print() const {
	std::string strPieces = "";

	for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
		for (File file = FILE_A; file <= FILE_H; ++file) {
			Square sq = (Square)(rank * 8 + file);
			strPieces += TypeConvertions::piece_to_char(piece_at_square(sq));
		}
	}

	for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
		std::string rankPieces = strPieces.substr(0, 8);
		strPieces = strPieces.substr(8);

		for (char& c : rankPieces) {
			std::cout << TypeConvertions::to_string(c) + " ";
		}
		std::cout << std::endl;
	}

	std::cout << std::endl;
}

void Position::add_piece(int squareNum, Piece piece) {
	Bitboard square = 1ULL << squareNum;
	Color color = piece <= wK ? WHITE : BLACK;

	if ((piece == wP) | (piece == bP)) add_pawn(color, square);
	if ((piece == wN) | (piece == bN)) add_knight(color, square);
	if ((piece == wB) | (piece == bB)) add_bishop(color, square);
	if ((piece == wR) | (piece == bR)) add_rook(color, square);
	if ((piece == wQ) | (piece == bQ)) add_queen(color, square);
	if ((piece == wK) | (piece == bK)) add_king(color, square);
}

void Position::add_pawn(Color color, Bitboard pawn) {
	OccupiedBB[color][PAWN] |= pawn;
	OccupiedBB[BOTH][PAWN] |= pawn;
	OccupiedBB[color][ANY_PIECE] |= pawn;
	OccupiedBB[BOTH][ANY_PIECE] |= pawn;

	pieces_[get_square(pawn)] = PAWN;
}

void Position::add_knight(Color color, Bitboard knight) {
	OccupiedBB[color][KNIGHT] |= knight;
	OccupiedBB[BOTH][KNIGHT] |= knight;
	OccupiedBB[color][ANY_PIECE] |= knight;
	OccupiedBB[BOTH][ANY_PIECE] |= knight;

	pieces_[get_square(knight)] = KNIGHT;
}

void Position::add_bishop(Color color, Bitboard bishop) {
	OccupiedBB[color][BISHOP] |= bishop;
	OccupiedBB[BOTH][BISHOP] |= bishop;
	OccupiedBB[color][ANY_PIECE] |= bishop;
	OccupiedBB[BOTH][ANY_PIECE] |= bishop;

	pieces_[get_square(bishop)] = BISHOP;
}

void Position::add_rook(Color color, Bitboard rook) {
	OccupiedBB[color][ROOK] |= rook;
	OccupiedBB[BOTH][ROOK] |= rook;
	OccupiedBB[color][ANY_PIECE] |= rook;
	OccupiedBB[BOTH][ANY_PIECE] |= rook;

	pieces_[get_square(rook)] = ROOK;
}

void Position::add_queen(Color color, Bitboard queen) {
	OccupiedBB[color][QUEEN] |= queen;
	OccupiedBB[BOTH][QUEEN] |= queen;
	OccupiedBB[color][ANY_PIECE] |= queen;
	OccupiedBB[BOTH][ANY_PIECE] |= queen;

	pieces_[get_square(queen)] = QUEEN;
}

void Position::add_king(Color color, Bitboard king) {
	OccupiedBB[color][KING] |= king;
	OccupiedBB[BOTH][KING] |= king;
	OccupiedBB[color][ANY_PIECE] |= king;
	OccupiedBB[BOTH][ANY_PIECE] |= king;

	kingSq_[color] = get_square(king);
	pieces_[get_square(king)] = KING;
}

void Position::clear_pieces() {
	for (int color = WHITE; color <= BOTH; ++color) {
		OccupiedBB[color][PAWN] = 0;
		OccupiedBB[color][KNIGHT] = 0;
		OccupiedBB[color][BISHOP] = 0;
		OccupiedBB[color][ROOK] = 0;
		OccupiedBB[color][QUEEN] = 0;
		OccupiedBB[color][KING] = 0;
		OccupiedBB[color][ANY_PIECE] = 0;

	}

	for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
		pieces_[sq] = NO_PIECE;
	}
}

Square Position::get_square(Bitboard sq64) {
	unsigned long square = 0;
	_BitScanForward64(&square, sq64);
	return (Square)square;
}

Piece Position::piece_at_square(Square sq) const {
	if (!(SquareBB[sq] & OccupiedBB[BOTH][ANY_PIECE])) return EMPTY;
	if (SquareBB[sq] & OccupiedBB[WHITE][PAWN]) return wP;
	if (SquareBB[sq] & OccupiedBB[BLACK][PAWN]) return bP;
	if (SquareBB[sq] & OccupiedBB[WHITE][KNIGHT]) return wN;
	if (SquareBB[sq] & OccupiedBB[BLACK][KNIGHT]) return bN;
	if (SquareBB[sq] & OccupiedBB[WHITE][BISHOP]) return wB;
	if (SquareBB[sq] & OccupiedBB[BLACK][BISHOP]) return bB;
	if (SquareBB[sq] & OccupiedBB[WHITE][ROOK]) return wR;
	if (SquareBB[sq] & OccupiedBB[BLACK][ROOK]) return bR;
	if (SquareBB[sq] & OccupiedBB[WHITE][QUEEN]) return wQ;
	if (SquareBB[sq] & OccupiedBB[BLACK][QUEEN]) return bQ;
	if (SquareBB[sq] & OccupiedBB[WHITE][KING]) return wK;
	if (SquareBB[sq] & OccupiedBB[BLACK][KING]) return bK;
	return EMPTY;
}

void Position::calculate_pos_key() {
	posKey_ = 0;

	for (int sq = 0; sq < 64; ++sq)
		if (!((OccupiedBB[BOTH][ANY_PIECE] >> sq) & 1)) continue; // empty square
		else if (((OccupiedBB[WHITE][PAWN] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[WHITE][PAWN][sq];
		else if (((OccupiedBB[BLACK][PAWN] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[BLACK][PAWN][sq];
		else if (((OccupiedBB[WHITE][KNIGHT] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[WHITE][KNIGHT][sq];
		else if (((OccupiedBB[BLACK][KNIGHT] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[BLACK][KNIGHT][sq];
		else if (((OccupiedBB[WHITE][BISHOP] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[WHITE][BISHOP][sq];
		else if (((OccupiedBB[BLACK][BISHOP] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[BLACK][BISHOP][sq];
		else if (((OccupiedBB[WHITE][ROOK] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[WHITE][ROOK][sq];
		else if (((OccupiedBB[BLACK][ROOK] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[BLACK][ROOK][sq];
		else if (((OccupiedBB[WHITE][QUEEN] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[WHITE][QUEEN][sq];
		else if (((OccupiedBB[BLACK][QUEEN] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[BLACK][QUEEN][sq];
		else if (((OccupiedBB[WHITE][KING] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[WHITE][KING][sq];
		else if (((OccupiedBB[BLACK][KING] >> sq) & 1) == 1) posKey_ ^= Zobrist::psq[BLACK][KING][sq];

	if (enPassant_)
		posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	
	posKey_ ^= Zobrist::castling[castlingRights_];
	
	if (sideToMove_) posKey_ ^= Zobrist::side;
}

bool Position::do_move(const Move move) {
	Square from = from_sq(move);
	Square to = to_sq(move);
	Color side = sideToMove_;
	Color enemySide = side == WHITE ? BLACK : WHITE;
	PieceType capt = captured(move);
	PieceType prom = promoted(move);

	history_[hisPly_].posKey = posKey_;

	if (move & FLAGEP) {
		if (side == WHITE) {
			clear_piece(to + SOUTH, BLACK);
		}
		else {
			clear_piece(to + NORTH, WHITE);
		}
	}
	else if (move & FLAGCA) {
		switch (to) {
		case SQ_C1: move_piece(SQ_A1, SQ_D1, WHITE); break;
		case SQ_G1: move_piece(SQ_H1, SQ_F1, WHITE); break;
		case SQ_C8: move_piece(SQ_A8, SQ_D8, BLACK); break;
		case SQ_G8: move_piece(SQ_H8, SQ_F8, BLACK); break;
		default:  assert(false); break;
		}
	}

	// Hash out current en pas square
	if (enPassant_) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	// Hash out current castling rights
	posKey_ ^= Zobrist::castling[castlingRights_];

	history_[hisPly_].move = move;
	history_[hisPly_].fiftyMove = fiftyMove_;
	history_[hisPly_].enPas = enPassant_;
	history_[hisPly_].castlePerm = castlingRights_;

	castlingRights_ &= CastlePerm[from];
	castlingRights_ &= CastlePerm[to];
	enPassant_ = SQ_NONE;

	// Hash in the new castling rights
	posKey_ ^= Zobrist::castling[castlingRights_];

	++fiftyMove_;

	if (capt) {
		clear_piece(to, enemySide);
		fiftyMove_ = 0;
	}

	++hisPly_;
	++ply_;

	if (pieces_[from] == PAWN) {
		fiftyMove_ = 0;
		if (move & FLAGPS) {
			if (side == WHITE) {
				enPassant_ = from + NORTH;
			}
			else {
				enPassant_ = from + SOUTH;
			}
			// Hash in new en passant square
			posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
		}
	}

	move_piece(from, to, side);

	if (prom) {
		clear_piece(to, side);
		add_piece(to, prom, side);
	}

	if (pieces_[to] == KING) {
		kingSq_[side] = to;
	}

	sideToMove_ = sideToMove_ == WHITE ? BLACK : WHITE;
	posKey_ ^= Zobrist::side;

	if (attackers_to(kingSq_[side]) & OccupiedBB[enemySide][ANY_PIECE]) {
		undo_move();
		return false;
	}
	return true;
}

void Position::undo_move() {
	--hisPly_;
	--ply_;

	Move move = history_[hisPly_].move;
	Square from = from_sq(move);
	Square to = to_sq(move);
	PieceType capt = captured(move);
	PieceType prom = promoted(move);

	if (enPassant_) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	posKey_ ^= Zobrist::castling[castlingRights_];

	castlingRights_ = history_[hisPly_].castlePerm;
	fiftyMove_ = history_[hisPly_].fiftyMove;
	enPassant_ = history_[hisPly_].enPas;

	if (enPassant_) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	posKey_ ^= Zobrist::castling[castlingRights_];

	sideToMove_ = sideToMove_ == WHITE ? BLACK : WHITE;
	posKey_ ^= Zobrist::side;

	if (move & FLAGEP) {
		if (sideToMove_ == WHITE) {
			add_piece(to + SOUTH, PAWN, BLACK);
		}
		else {
			add_piece(to + NORTH, PAWN, WHITE);
		}
	}
	else if (move & FLAGCA) {
		switch (to) {
		case SQ_C1: move_piece(SQ_D1, SQ_A1, WHITE); break;
		case SQ_G1: move_piece(SQ_F1, SQ_H1, WHITE); break;
		case SQ_C8: move_piece(SQ_D8, SQ_A8, BLACK); break;
		case SQ_G8: move_piece(SQ_F8, SQ_H8, BLACK); break;
		default: /*ERROR*/; break;
		}
	}

	move_piece(to, from, sideToMove_);

	if (pieces_[from] == KING) {
		kingSq_[sideToMove_] = from;
	}

	if (capt) {
		add_piece(to, capt, sideToMove_ ^ 1);
	}

	if (prom) {
		clear_piece(from, sideToMove_);
		add_piece(from, PAWN, sideToMove_);
	}
}

void Position::clear_piece(const Square sq, const int pieceColor) {
	PieceType piece = pieces_[sq];

	// Remove piece from posKey
	posKey_ ^= Zobrist::psq[pieceColor][piece][sq];
	// Remove piece from piecelist
	pieces_[sq] = NO_PIECE;

	// Remove piece from bitboards
	clear_bit(OccupiedBB[pieceColor][piece], sq);
	clear_bit(OccupiedBB[pieceColor][ANY_PIECE], sq);
	clear_bit(OccupiedBB[BOTH][piece], sq);
	clear_bit(OccupiedBB[BOTH][ANY_PIECE], sq);
}

void Position::add_piece(const Square sq, const PieceType piece, const int pieceColor) {
	// Add piece to posKey
	posKey_ ^= Zobrist::psq[pieceColor][piece][sq];
	// Add piece to piecelist
	pieces_[sq] = piece;

	// Add piece to bitboards
	set_bit(OccupiedBB[pieceColor][piece], sq);
	set_bit(OccupiedBB[pieceColor][ANY_PIECE], sq);
	set_bit(OccupiedBB[BOTH][piece], sq);
	set_bit(OccupiedBB[BOTH][ANY_PIECE], sq);
}

void Position::move_piece(const Square from, const Square to, const int pieceColor) {
	PieceType piece = pieces_[from];

	// Remove piece from posKey
	posKey_ ^= Zobrist::psq[pieceColor][piece][from];
	// Remove piece from piecelist
	pieces_[from] = NO_PIECE;

	// Add piece to posKey
	posKey_ ^= Zobrist::psq[pieceColor][piece][to];
	// Add piece to piecelist
	pieces_[to] = piece;

	// Remove piece from bitboards
	clear_bit(OccupiedBB[pieceColor][piece], from);
	clear_bit(OccupiedBB[pieceColor][ANY_PIECE], from);
	clear_bit(OccupiedBB[BOTH][piece], from);
	clear_bit(OccupiedBB[BOTH][ANY_PIECE], from);

	// Add piece to bitboards
	set_bit(OccupiedBB[pieceColor][piece], to);
	set_bit(OccupiedBB[pieceColor][ANY_PIECE], to);
	set_bit(OccupiedBB[BOTH][piece], to);
	set_bit(OccupiedBB[BOTH][ANY_PIECE], to);
}