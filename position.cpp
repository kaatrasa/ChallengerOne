#include <iostream>
#include <random>

#include "position.h"
#include "movegen.h"
#include "psqt.h"
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
	for (Rank r = RANK_8; r >= RANK_1; --r) {
		pos = fen.find("/");
		string fenRow = fen.substr(0, pos).substr(0, fen.find(" "));
		fen = fen.substr(pos + 1);

		File f = FILE_A;
		for (char& c : fenRow) {
			if (isdigit(c)) {
				int newFile = f + TypeConvertions::to_int(c);
				f = File(newFile);
			} else {
				Square s = make_square(f, r);
				add_piece(TypeConvertions::char_to_piece(c), s);

				++f;
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
	bool found;
	TTEntry* entry = TT.probe(posKey_, found);

	if (found) return entry->move;
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

void Position::print_pv(SearchInfo& info, const Depth depth) {
	int pvCount = pv(depth);
	int pvIndex = 0;
	std::cout << " pv ";
	while (pvCount) {
		Move pvMove = pvArray_[pvIndex];
		std::cout << TypeConvertions::move_to_string(pvMove) << " ";
		--pvCount;
		++pvIndex;
	}
	std::cout << std::endl;
}

bool Position::is_real_move(Move move) {
	Movelist list = Movelist();
	Movegen::get_moves(*this, list);

	for (int moveNum = 0; moveNum < list.count; ++moveNum)
		if (list.moves[moveNum].move == move) {
			if (!do_move(move)) return false;
			undo_move();
			
			return true;
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
	std::cout << "Key: " << posKey_ << std::endl;

	std::cout << std::endl;
}

void Position::add_piece(Piece pc, Square s) {
	Color c = pc <= wK ? WHITE : BLACK;

	if ((pc == wP) | (pc == bP)) add_pawn(c, s);
	if ((pc == wN) | (pc == bN)) add_knight(c, s);
	if ((pc == wB) | (pc == bB)) add_bishop(c, s);
	if ((pc == wR) | (pc == bR)) add_rook(c, s);
	if ((pc == wQ) | (pc == bQ)) add_queen(c, s);
	if ((pc == wK) | (pc == bK)) add_king(c, s);
}

void Position::add_pawn(Color c, Square s) {
	OccupiedBB[c][PAWN] |= SquareBB[s];
	OccupiedBB[BOTH][PAWN] |= SquareBB[s];
	OccupiedBB[c][ANY_PIECE] |= SquareBB[s];
	OccupiedBB[BOTH][ANY_PIECE] |= SquareBB[s];

	pieces_[s] = PAWN;
	psq_[PHASE_MID] += c == WHITE ? PawnValueMg : -PawnValueMg;
	psq_[PHASE_END] += c == WHITE ? PawnValueEg : -PawnValueEg;
}

void Position::add_knight(Color c, Square s) {
	OccupiedBB[c][KNIGHT] |= SquareBB[s];
	OccupiedBB[BOTH][KNIGHT] |= SquareBB[s];
	OccupiedBB[c][ANY_PIECE] |= SquareBB[s];
	OccupiedBB[BOTH][ANY_PIECE] |= SquareBB[s];

	pieces_[s] = KNIGHT;
	psq_[PHASE_MID] += c == WHITE ? KnightValueMg : -KnightValueMg;
	psq_[PHASE_END] += c == WHITE ? KnightValueEg : -KnightValueEg;
	nonPawnMaterial_[c] += KnightValueMg;
}

void Position::add_bishop(Color c, Square s) {
	OccupiedBB[c][BISHOP] |= SquareBB[s];
	OccupiedBB[BOTH][BISHOP] |= SquareBB[s];
	OccupiedBB[c][ANY_PIECE] |= SquareBB[s];
	OccupiedBB[BOTH][ANY_PIECE] |= SquareBB[s];

	pieces_[s] = BISHOP;
	psq_[PHASE_MID] += c == WHITE ? BishopValueMg : -BishopValueMg;
	psq_[PHASE_END] += c == WHITE ? BishopValueEg : -BishopValueEg;
	nonPawnMaterial_[c] += BishopValueMg;
}

void Position::add_rook(Color c, Square s) {
	OccupiedBB[c][ROOK] |= SquareBB[s];
	OccupiedBB[BOTH][ROOK] |= SquareBB[s];
	OccupiedBB[c][ANY_PIECE] |= SquareBB[s];
	OccupiedBB[BOTH][ANY_PIECE] |= SquareBB[s];

	pieces_[s] = ROOK;
	psq_[PHASE_MID] += c == WHITE ? RookValueMg : -RookValueMg;
	psq_[PHASE_END] += c == WHITE ? RookValueEg : -RookValueEg;
	nonPawnMaterial_[c] += RookValueMg;
}

void Position::add_queen(Color c, Square s) {
	OccupiedBB[c][QUEEN] |= SquareBB[s];
	OccupiedBB[BOTH][QUEEN] |= SquareBB[s];
	OccupiedBB[c][ANY_PIECE] |= SquareBB[s];
	OccupiedBB[BOTH][ANY_PIECE] |= SquareBB[s];

	pieces_[s] = QUEEN;
	psq_[PHASE_MID] += c == WHITE ? QueenValueMg : -QueenValueMg;
	psq_[PHASE_END] += c == WHITE ? QueenValueEg : -QueenValueEg;
	nonPawnMaterial_[c] += QueenValueMg;
}

void Position::add_king(Color c, Square s) {
	OccupiedBB[c][KING] |= SquareBB[s];
	OccupiedBB[BOTH][KING] |= SquareBB[s];
	OccupiedBB[c][ANY_PIECE] |= SquareBB[s];
	OccupiedBB[BOTH][ANY_PIECE] |= SquareBB[s];

	kingSq_[c] = s;
	pieces_[s] = KING;
}

void Position::clear_pieces() {
	for (Color c = WHITE; c <= BOTH; ++c) {
		OccupiedBB[c][PAWN] = 0;
		OccupiedBB[c][KNIGHT] = 0;
		OccupiedBB[c][BISHOP] = 0;
		OccupiedBB[c][ROOK] = 0;
		OccupiedBB[c][QUEEN] = 0;
		OccupiedBB[c][KING] = 0;
		OccupiedBB[c][ANY_PIECE] = 0;
	}

	for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
		pieces_[sq] = NO_PIECE;

	for (Phase p = PHASE_MID; p <= PHASE_END; ++p)
		psq_[p] = VALUE_ZERO;

	for (Color c = WHITE; c <= BLACK; ++c)
		nonPawnMaterial_[COLOR_NB] = VALUE_ZERO;
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

	if (enPassant_ != SQ_NONE)
		posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	
	posKey_ ^= Zobrist::castling[castlingRights_];
	
	if (sideToMove_) posKey_ ^= Zobrist::side;
}

bool Position::do_move(const Move move) {
	Square from = from_sq(move);
	Square to = to_sq(move);
	Color side = sideToMove_;
	Color enemySide = side == WHITE ? BLACK : WHITE;
	PieceType capt = captured_piece(move);
	PieceType prom = promoted_piece(move);

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
	if (enPassant_ != SQ_NONE) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
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
	PieceType capt = captured_piece(move);
	PieceType prom = promoted_piece(move);

	if (enPassant_ != SQ_NONE) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	posKey_ ^= Zobrist::castling[castlingRights_];

	castlingRights_ = history_[hisPly_].castlePerm;
	fiftyMove_ = history_[hisPly_].fiftyMove;
	enPassant_ = history_[hisPly_].enPas;

	if (enPassant_ != SQ_NONE) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
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

	if (pieces_[from] == KING)
		kingSq_[sideToMove_] = from;

	if (capt)
		add_piece(to, capt, ~sideToMove_);

	if (prom) {
		clear_piece(from, sideToMove_);
		add_piece(from, PAWN, sideToMove_);
	}
}

void Position::do_null_move() {
	++ply_;
	history_[hisPly_].posKey = posKey_;

	if (enPassant_ != SQ_NONE) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];

	history_[hisPly_].move = MOVE_NONE;
	history_[hisPly_].fiftyMove = fiftyMove_;
	history_[hisPly_].enPas = enPassant_;
	history_[hisPly_].castlePerm = castlingRights_;
	enPassant_ = SQ_NONE;

	sideToMove_ = sideToMove_ == WHITE ? BLACK : WHITE;
	++hisPly_;
	posKey_ ^= Zobrist::side;
}

void Position::undo_null_move() {
	--hisPly_;
	--ply_;

	if (enPassant_ != SQ_NONE) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];

	castlingRights_ = history_[hisPly_].castlePerm;
	fiftyMove_ = history_[hisPly_].fiftyMove;
	enPassant_ = history_[hisPly_].enPas;

	if (enPassant_ != SQ_NONE) posKey_ ^= Zobrist::enpassant[enPassant_ & 7];
	sideToMove_ = sideToMove_ == WHITE ? BLACK : WHITE;
	posKey_ ^= Zobrist::side;
}

void Position::clear_piece(const Square s, const Color c) {
	PieceType pt = pieces_[s];

	// Remove piece from posKey
	posKey_ ^= Zobrist::psq[c][pt][s];
	// Remove piece from piecelist
	pieces_[s] = NO_PIECE;

	// Remove piece from bitboards
	clear_bit(OccupiedBB[c][pt], s);
	clear_bit(OccupiedBB[c][ANY_PIECE], s);
	clear_bit(OccupiedBB[BOTH][pt], s);
	clear_bit(OccupiedBB[BOTH][ANY_PIECE], s);

	// Update psq
	psq_[PHASE_MID] -= PSQT::psq[c][pt][s][PHASE_MID];
	psq_[PHASE_END] -= PSQT::psq[c][pt][s][PHASE_END];

	// Update nonpawn material
	if (pt != KING && pt != PAWN)
		nonPawnMaterial_[c] -= PSQT::PieceValue[PHASE_MID][pt];
}

void Position::add_piece(const Square s, const PieceType pt, const Color c) {
	// Add piece to posKey
	posKey_ ^= Zobrist::psq[c][pt][s];
	// Add piece to piecelist
	pieces_[s] = pt;

	// Add piece to bitboards
	set_bit(OccupiedBB[c][pt], s);
	set_bit(OccupiedBB[c][ANY_PIECE], s);
	set_bit(OccupiedBB[BOTH][pt], s);
	set_bit(OccupiedBB[BOTH][ANY_PIECE], s);

	// Update psq
	psq_[PHASE_MID] += PSQT::psq[c][pt][s][PHASE_MID];
	psq_[PHASE_END] += PSQT::psq[c][pt][s][PHASE_END];

	// Update nonpawn material
	if (pt != KING && pt != PAWN)
		nonPawnMaterial_[c] += PSQT::PieceValue[PHASE_MID][pt];
}

void Position::move_piece(const Square from, const Square to, const Color c) {
	PieceType pt = pieces_[from];

	// Remove piece from posKey
	posKey_ ^= Zobrist::psq[c][pt][from];
	// Remove piece from piecelist
	pieces_[from] = NO_PIECE;

	// Add piece to posKey
	posKey_ ^= Zobrist::psq[c][pt][to];
	// Add piece to piecelist
	pieces_[to] = pt;

	// Remove piece from bitboards
	clear_bit(OccupiedBB[c][pt], from);
	clear_bit(OccupiedBB[c][ANY_PIECE], from);
	clear_bit(OccupiedBB[BOTH][pt], from);
	clear_bit(OccupiedBB[BOTH][ANY_PIECE], from);

	// Add piece to bitboards
	set_bit(OccupiedBB[c][pt], to);
	set_bit(OccupiedBB[c][ANY_PIECE], to);
	set_bit(OccupiedBB[BOTH][pt], to);
	set_bit(OccupiedBB[BOTH][ANY_PIECE], to);

	// Update psq
	psq_[PHASE_MID] -= PSQT::psq[c][pt][from][PHASE_MID];
	psq_[PHASE_END] -= PSQT::psq[c][pt][from][PHASE_END];
	psq_[PHASE_MID] += PSQT::psq[c][pt][to][PHASE_MID];
	psq_[PHASE_END] += PSQT::psq[c][pt][to][PHASE_END];
}