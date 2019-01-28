#include <iostream>
#include <intrin.h>
#include <algorithm>

#include "bitboard.h"
#include "utils/defs.h"

Bitboard OccupiedBB[3][7];
Bitboard KnightAttacks[SQUARE_CNT], KingAttacks[SQUARE_CNT], PawnAttacksEast[2][SQUARE_CNT], PawnAttacksWest[2][SQUARE_CNT];
Bitboard SetMask[64], ClearMask[64];

void init_pawn_attacks() {
	for (Square s = SQ_A1; s <= SQ_H8; ++s) {
		File f = file_of(s);

		if (f == FILE_A) {
			if (is_ok(s + NORTH_EAST))
				PawnAttacksEast[WHITE][s] = SquareBB[s + NORTH_EAST];

			if (is_ok(s + SOUTH_EAST))
				PawnAttacksEast[BLACK][s] = SquareBB[s + SOUTH_EAST];

		} else if (f == FILE_H) {
			if (is_ok(s + NORTH_WEST))
				PawnAttacksWest[WHITE][s] = SquareBB[s + NORTH_WEST];

			if (is_ok(s + SOUTH_WEST))
				PawnAttacksWest[BLACK][s] = SquareBB[s + SOUTH_WEST];

		} else {
			if (is_ok(s + NORTH_EAST))
				PawnAttacksEast[WHITE][s] = SquareBB[s + NORTH_EAST];

			if (is_ok(s + NORTH_WEST))
				PawnAttacksWest[WHITE][s] = SquareBB[s + NORTH_WEST];

			if (is_ok(s + SOUTH_EAST))
				PawnAttacksEast[BLACK][s] = SquareBB[s + SOUTH_EAST];

			if (is_ok(s + SOUTH_WEST))
				PawnAttacksWest[BLACK][s] = SquareBB[s + SOUTH_WEST];
		}
	}
}

void init_knight_attacks() {
	int jumps[8] = { -6, -10, -15, -17, 6, 10, 15, 17 };

	for (Square s = SQ_A1; s <= SQ_H8; ++s) {
		Bitboard attacks = 0;

		for (int jump : jumps) {
			Square to = s + Direction(jump);

			if (is_ok(to) && SquareDistance[s][to] < 3) {
				attacks |= SquareBB[to];
			}
		}

		KnightAttacks[s] = attacks;
	}
}

void init_king_attacks() {
	for (Square s = SQ_A1; s <= SQ_H8; ++s) {
		Bitboard attacks = 0;
		File f = file_of(s);
		Rank r = rank_of(s);

		if (is_ok(s + NORTH))
			attacks = SquareBB[s + NORTH];

		if (is_ok(s + SOUTH))
			attacks |= SquareBB[s + SOUTH];

		if (is_ok(s + EAST) && SquareDistance[s][s + EAST] == 1)
			attacks |= SquareBB[s + EAST];

		if (is_ok(s + WEST) && SquareDistance[s][s + WEST] == 1)
			attacks |= SquareBB[s + WEST];

		if (is_ok(s + NORTH_EAST) && SquareDistance[s][s + NORTH_EAST] == 1)
			attacks |= SquareBB[s + NORTH_EAST];

		if (is_ok(s + NORTH_WEST) && SquareDistance[s][s + NORTH_WEST] == 1)
			attacks |= SquareBB[s + NORTH_WEST];

		if (is_ok(s + SOUTH_EAST) && SquareDistance[s][s + SOUTH_EAST] == 1)
			attacks |= SquareBB[s + SOUTH_EAST];

		if (is_ok(s + SOUTH_WEST) && SquareDistance[s][s + SOUTH_WEST] == 1)
			attacks |= SquareBB[s + SOUTH_WEST];

		KingAttacks[s] = attacks;
	}
}

int SquareDistance[SQUARE_CNT][SQUARE_CNT];
Bitboard SquareBB[SQUARE_CNT];
Bitboard FileBB[FILE_CNT];
Bitboard RankBB[RANK_CNT];

Magic RookMagics[SQUARE_CNT];
Magic BishopMagics[SQUARE_CNT];

namespace {

	Bitboard RookTable[0x19000];  // To store rook attacks
	Bitboard BishopTable[0x1480]; // To store bishop attacks

	void init_magics(Bitboard table[], Magic magics[], Direction directions[]);

	// popcount16() counts the non-zero bits using SWAR-Popcount algorithm
	unsigned popcount16(unsigned u) {
		u -= (u >> 1) & 0x5555U;
		u = ((u >> 2) & 0x3333U) + (u & 0x3333U);
		u = ((u >> 4) + u) & 0x0F0FU;
		return (u * 0x0101U) >> 8;
	}
}

void init_bitboards() {
	for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
		for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
			if (s1 != s2) SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));

	for (Color c = WHITE; c <= BOTH; ++c)
		for (PieceType p = ANY_PIECE; p <= KING; ++p)
			OccupiedBB[c][p] = 0;

	for (Square s = SQ_A1; s <= SQ_H8; ++s) {
		SquareBB[s] = (1ULL << s);
		SetMask[s] = (1ULL << s);
		ClearMask[s] = ~SetMask[s];
	}

	for (File f = FILE_A; f <= FILE_H; ++f)
		FileBB[f] = f > FILE_A ? FileBB[f - 1] << 1 : FileABB;

	for (Rank r = RANK_1; r <= RANK_8; ++r)
		RankBB[r] = r > RANK_1 ? RankBB[r - 1] << 8 : Rank1BB;

	Direction RookDirections[] = { NORTH, EAST, SOUTH, WEST };
	Direction BishopDirections[] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };
	
	init_pawn_attacks();
	init_knight_attacks();
	init_king_attacks();
	init_magics(RookTable, RookMagics, RookDirections);
	init_magics(BishopTable, BishopMagics, BishopDirections);
}

Bitboard single_push_targets_white(Bitboard wpawns, Bitboard empty) {
	return north_one(wpawns) & empty;
}

Bitboard double_push_targets_white(Bitboard wpawns, Bitboard empty) {
	const Bitboard rank4 = 0x00000000FF000000;
	Bitboard singlePushs = single_push_targets_white(wpawns, empty);
	return north_one(singlePushs) & empty & rank4;
}

Bitboard single_push_targets_black(Bitboard bpawns, Bitboard empty) {
	return south_one(bpawns) & empty;
}

Bitboard double_push_targets_black(Bitboard bpawns, Bitboard empty) {
	const Bitboard rank5 = 0x000000FF00000000;
	Bitboard singlePushs = single_push_targets_black(bpawns, empty);
	return south_one(singlePushs) & empty & rank5;
}

namespace {
	Bitboard sliding_attack(Direction directions[], Square sq, Bitboard occupied) {
		Bitboard attack = 0;

		for (int i = 0; i < 4; ++i)
			for (Square s = sq + directions[i];
				is_ok(s) && distance(s, s - directions[i]) == 1;
				s += directions[i])
		{
			attack |= s;

			if (occupied & s)
				break;
		}

		return attack;
	}

	// init_magics() computes all rook and bishop attacks at startup. Magic
	// bitboards are used to look up attacks of sliding pieces. As a reference see
	// chessprogramming.wikispaces.com/Magic+Bitboards. In particular, here we
	// use the so called "fancy" approach.
	void init_magics(Bitboard table[], Magic magics[], Direction directions[]) {

		// Optimal PRNG seeds to pick the correct magics in the shortest time
		int seeds[][RANK_CNT] = { { 8977, 44560, 54343, 38998,  5731, 95205, 104912, 17020 },
		{ 728, 10316, 55013, 32803, 12281, 15100,  16645,   255 } };

		Bitboard occupancy[4096], reference[4096], edges, b;
		int epoch[4096] = {}, cnt = 0, size = 0;

		for (Square s = SQ_A1; s <= SQ_H8; ++s)
		{
			// Board edges are not considered in the relevant occupancies
			edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));

			// Given a square 's', the mask is the bitboard of sliding attacks from
			// 's' computed on an empty board. The index must be big enough to contain
			// all the attacks for each possible subset of the mask and so is 2 power
			// the number of 1s of the mask. Hence we deduce the size of the shift to
			// apply to the 64 or 32 bits word to get the index.
			Magic& m = magics[s];
			m.mask = sliding_attack(directions, s, 0) & ~edges;

			// Set the offset for the attacks table of the square. We have individual
			// table sizes for each square with "Fancy Magic Bitboards".
			m.attacks = s == SQ_A1 ? table : magics[s - 1].attacks + size;

			// Use Carry-Rippler trick to enumerate all subsets of masks[s] and
			// store the corresponding sliding attack bitboard in reference[].
			b = size = 0;
			do {
				occupancy[size] = b;
				reference[size] = sliding_attack(directions, s, b);

				m.attacks[_pext_u64(b, m.mask)] = reference[size];

				size++;
				b = (b - m.mask) & m.mask;
			} while (b);
		}
	}
}