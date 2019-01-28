#pragma once

#include <assert.h>
#include <vector>
#include "utils/defs.h"

// Magic holds all magic bitboards relevant data for a single square
struct Magic {
	Bitboard  mask;
	Bitboard* attacks;

	// Compute the attack's index using the 'magic bitboards' approach
	unsigned index(Bitboard occ) const {
		return unsigned(pext(occ, mask));
	}
};

extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];

const Bitboard AllSquares = ~Bitboard(0);
const Bitboard DarkSquares = 0xAA55AA55AA55AA55ULL;
const Bitboard NotAFile = 0xfefefefefefefefe; // ~0x0101010101010101
const Bitboard NotHFile = 0x7f7f7f7f7f7f7f7f; // ~0x8080808080808080
constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

constexpr Bitboard Rank1BB = 0xFF;
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

extern Bitboard OccupiedBB[3][7]; // BB[color][piecetype]
extern Bitboard KnightAttacks[SQUARE_NB], KingAttacks[SQUARE_NB], PawnAttacksEast[2][SQUARE_NB], PawnAttacksWest[2][SQUARE_NB]; // [color][sq]

extern int SquareDistance[SQUARE_NB][SQUARE_NB];
extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard FileBB[FILE_NB];
extern Bitboard RankBB[RANK_NB];

void init_bitboards();

template<PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occ) {
	assert(Pt != PAWN);
	const Magic& m = Pt == ROOK ? RookMagics[s] : BishopMagics[s];
	return m.attacks[m.index(occ)];
}

inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occ) {
	assert(pt != PAWN);
	switch (pt)
	{
	case BISHOP: return attacks_bb<BISHOP>(s, occ);
	case ROOK: return attacks_bb<  ROOK>(s, occ);
	case QUEEN: return attacks_bb<BISHOP>(s, occ) | attacks_bb<ROOK>(s, occ);
	case KNIGHT: return KnightAttacks[s];
	case KING: return KingAttacks[s];
	}
}

inline Bitboard north_one(Bitboard bb) { return  bb << 8; }
inline Bitboard south_one(Bitboard bb) { return  bb >> 8; }
inline Bitboard east_one(Bitboard bb) { return (bb << 1) & NotAFile; }
inline Bitboard west_one(Bitboard bb) { return (bb >> 1) & NotHFile; }

Bitboard single_push_targets_white(Bitboard wpawns, Bitboard empty);
Bitboard double_push_targets_white(Bitboard wpawns, Bitboard empty);
Bitboard single_push_targets_black(Bitboard bpawns, Bitboard empty);
Bitboard double_push_targets_black(Bitboard bpawns, Bitboard empty);

extern Bitboard SetMask[64];
extern Bitboard ClearMask[64];

inline void set_bit(Bitboard& b, Square s) {
	b |= SquareBB[s];
}

inline void clear_bit(Bitboard& b, Square s) {
	b &= ClearMask[s];
}

// Overloads of bitwise operators between a Bitboard and a Square for testing
// whether a given bit is set in a bitboard, and for setting and clearing bits.
inline Bitboard operator&(Bitboard b, Square s) {
	assert(s >= SQ_A1 && s <= SQ_H8);
	return b & SquareBB[s];
}

inline Bitboard operator|(Bitboard b, Square s) {
	assert(s >= SQ_A1 && s <= SQ_H8);
	return b | SquareBB[s];
}

inline Bitboard operator^(Bitboard b, Square s) {
	assert(s >= SQ_A1 && s <= SQ_H8);
	return b ^ SquareBB[s];
}

inline Bitboard& operator|=(Bitboard& b, Square s) {
	assert(s >= SQ_A1 && s <= SQ_H8);
	return b |= SquareBB[s];
}

inline Bitboard& operator^=(Bitboard& b, Square s) {
	assert(s >= SQ_A1 && s <= SQ_H8);
	return b ^= SquareBB[s];
}

inline Bitboard rank_bb(Rank r) {
	return RankBB[r];
}

inline Bitboard rank_bb(Square s) {
	return RankBB[rank_of(s)];
}

inline Bitboard file_bb(File f) {
	return FileBB[f];
}

inline Bitboard file_bb(Square s) {
	return FileBB[file_of(s)];
}

// distance() functions return the distance between x and y, defined as the
// number of steps for a king in x to reach y. Works with squares, ranks, files.
template<typename T> inline int distance(T x, T y) { return x < y ? y - x : x - y; }
template<> inline int distance<Square>(Square x, Square y) { return SquareDistance[x][y]; }

template<typename T1, typename T2> inline int distance(T2 x, T2 y);
template<> inline int distance<File>(Square x, Square y) { return distance(file_of(x), file_of(y)); }
template<> inline int distance<Rank>(Square x, Square y) { return distance(rank_of(x), rank_of(y)); }
