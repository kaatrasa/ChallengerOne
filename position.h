#pragma once

#include <string>

#include "position.h"
#include "bitboard.h"
#include "utils/defs.h"

namespace Zobrist {
	void init_keys();
}

class Position {
public:
	Position();

	void set(std::string fen);
	const std::string fen() const;

	void print() const;

	bool do_move(const Move move);
	void undo_move();
	void do_null_move();
	void undo_null_move();

	Key pos_key() const;
	Color side_to_move() const;
	CastlingRight castling_rights() const;
	bool can_castle(CastlingRight cr) const;
	int fifty_move() const;
	Square en_passant() const;
	Square king_sq() const;
	Square king_sq(Color side) const;
	PieceType piece_on_sq(int sq) const;
	Bitboard non_pawn_material(Color side);
	bool advanced_pawn_push(Move m) const;

	int ply() const;
	void ply_reset();
	int his_ply() const;
	void his_ply_reset();

	bool is_repetition();
	Move best_move();
	void print_pv(SearchInfo& info, const Depth depth);

	// Move ordering, non captures
	int history_move(Move move) const;
	void history_move_set(Move move, int value);
	void history_moves_reset();
	int killer_move1() const;
	int killer_move2() const;
	void killer_move_set(int move);
	void killer_moves_reset();

	// Attacks to/from a given square
	Bitboard attackers_to(Square s) const;
	Bitboard attackers_to(Square s, Bitboard occ) const;
	Bitboard attacks_from(PieceType pt, Square s) const;
	template<PieceType> Bitboard attacks_from(Square s) const;
	template<PieceType> Bitboard attacks_from(Square s, Color c) const;

private:
	Piece piece_at_square(Square sq) const;
	void calculate_pos_key();
	
	int pv(const int depth);
	bool is_real_move(Move move);

	// do_move, undo_move
	void clear_piece(const Square sq, const int pieceColor);
	void add_piece(const Square sq, const PieceType piece, const int pieceColor);
	void move_piece(const Square from, const Square to, const int pieceColor);

	// fen
	void add_piece(int squareNum, Piece piece);
	void add_pawn(Color color, Bitboard pawn);
	void add_knight(Color color, Bitboard knight);
	void add_bishop(Color color, Bitboard bishop);
	void add_rook(Color color, Bitboard rook);
	void add_queen(Color color, Bitboard queen);
	void add_king(Color color, Bitboard king);
	void clear_pieces();

	std::string fen_;
	Color sideToMove_ = WHITE;
	Key posKey_ = 0;

	Square enPassant_ = SQ_NONE;
	CastlingRight castlingRights_ = NO_CASTLING;
	int fiftyMove_ = 0;
	int ply_ = 0;
	int hisPly_ = 0;

	Undo history_[MAX_GAMELENGTH];
	Square kingSq_[BOTH];
	PieceType pieces_[SQUARE_NB];
	Move pvArray_[DEPTH_MAX];
	Value psq;

	// Move ordering, non captures
	int historyMoves_[2][SQUARE_NB][SQUARE_NB]; // [color][sq][sq]
	int killerMoves_[2][DEPTH_MAX]; // [killercount == 2][ply]

};

inline Color Position::side_to_move() const {
	return sideToMove_;
}

inline CastlingRight Position::castling_rights() const {
	return castlingRights_;
}

inline bool Position::can_castle(CastlingRight cr) const {
	return castlingRights_ & cr;
}

inline int Position::fifty_move() const {
	return fiftyMove_;
}

inline Square Position::en_passant() const {
	return enPassant_;
}

inline Square Position::king_sq(Color side) const {
	return kingSq_[side];
}

inline Square Position::king_sq() const {
	return kingSq_[sideToMove_];
}

inline PieceType Position::piece_on_sq(int sq) const {
	return pieces_[sq];
}

inline Bitboard Position::non_pawn_material(Color side) {
	return OccupiedBB[side][ANY_PIECE] ^ OccupiedBB[side][PAWN] ^ OccupiedBB[side][KING];
}

inline int Position::ply() const {
	return ply_;
}

inline void Position::ply_reset() {
	ply_ = 0;
}

inline int Position::his_ply() const {
	return hisPly_;
}

inline void Position::his_ply_reset() {
	hisPly_ = 0;
}

inline Key Position::pos_key() const {
	return posKey_;
}

inline int Position::history_move(Move move) const {
	return historyMoves_[sideToMove_][from_sq(move)][to_sq(move)];
}

inline void Position::history_move_set(Move move, int value) {
	historyMoves_[sideToMove_][from_sq(move)][to_sq(move)] += value;
}

inline void Position::history_moves_reset() {
	for (Color c = WHITE; c <= BLACK; ++c)
		for (Square from = SQ_A1; from <= SQ_H8; ++from)
			for (Square to = SQ_A1; to <= SQ_H8; ++to)
				historyMoves_[c][from][to] = 0;
}

inline int Position::killer_move1() const {
	return killerMoves_[0][ply_];
}

inline int Position::killer_move2() const {
	return killerMoves_[1][ply_];
}

inline void Position::killer_move_set(int move) {
	killerMoves_[1][ply_] = killerMoves_[0][ply_];
	killerMoves_[0][ply_] = move;
}

inline void Position::killer_moves_reset() {
	for (int ply = 0; ply < DEPTH_MAX; ++ply) {
		killerMoves_[0][ply] = MOVE_NONE;
		killerMoves_[1][ply] = MOVE_NONE;
	}
}

inline bool Position::advanced_pawn_push(Move m) const {
	return sideToMove_ == WHITE ? rank_of(from_sq(m)) > RANK_4
		: rank_of(from_sq(m)) < RANK_5;
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const {
	assert(Pt != PAWN);
	return  Pt == BISHOP || Pt == ROOK ? attacks_bb<Pt>(s, OccupiedBB[BOTH][ANY_PIECE])
		: Pt == QUEEN ? attacks_from<ROOK>(s) | attacks_from<BISHOP>(s)
		: Pt == KNIGHT ? KnightAttacks[s]
		: KingAttacks[s];
}

template<>
inline Bitboard Position::attacks_from<PAWN>(Square s, Color c) const {
	return (PawnAttacksEast[c][s] | PawnAttacksWest[c][s]);
}

inline Bitboard Position::attacks_from(PieceType pt, Square s) const {
	assert(pt != PAWN);
	return attacks_bb(pt, s, OccupiedBB[BOTH][ANY_PIECE]);
}

inline Bitboard Position::attackers_to(Square s) const {
	return attackers_to(s, OccupiedBB[BOTH][ANY_PIECE]);
}