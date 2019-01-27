#include "evaluate.h"

namespace Evaluation {
	
	const int PawnTable[64] = {
		0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,
		10	,	10	,	0	,	-10	,	-10	,	0	,	10	,	10	,
		5	,	0	,	0	,	5	,	5	,	0	,	0	,	5	,
		0	,	0	,	10	,	20	,	20	,	10	,	0	,	0	,
		5	,	5	,	5	,	10	,	10	,	5	,	5	,	5	,
		10	,	10	,	10	,	20	,	20	,	10	,	10	,	10	,
		20	,	20	,	20	,	30	,	30	,	20	,	20	,	20	,
		0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
	};

	const int KnightTable[64] = {
		0	,	-10	,	0	,	0	,	0	,	0	,	-10	,	0	,
		0	,	0	,	0	,	5	,	5	,	0	,	0	,	0	,
		0	,	0	,	10	,	10	,	10	,	10	,	0	,	0	,
		0	,	0	,	10	,	20	,	20	,	10	,	5	,	0	,
		5	,	10	,	15	,	20	,	20	,	15	,	10	,	5	,
		5	,	10	,	10	,	20	,	20	,	10	,	10	,	5	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
	};

	const int BishopTable[64] = {
		0	,	0	,	-10	,	0	,	0	,	-10	,	0	,	0	,
		0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
		0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
		0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
		0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
		0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
		0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
		0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
	};

	const int RookTable[64] = {
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
		25	,	25	,	25	,	25	,	25	,	25	,	25	,	25	,
		0	,	0	,	5	,	10	,	10	,	5	,	0	,	0
	};

	const int Mirror64[64] = {
		56	,	57	,	58	,	59	,	60	,	61	,	62	,	63	,
		48	,	49	,	50	,	51	,	52	,	53	,	54	,	55	,
		40	,	41	,	42	,	43	,	44	,	45	,	46	,	47	,
		32	,	33	,	34	,	35	,	36	,	37	,	38	,	39	,
		24	,	25	,	26	,	27	,	28	,	29	,	30	,	31	,
		16	,	17	,	18	,	19	,	20	,	21	,	22	,	23	,
		8	,	9	,	10	,	11	,	12	,	13	,	14	,	15	,
		0	,	1	,	2	,	3	,	4	,	5	,	6	,	7
	};

	Value evaluate(const Position& pos) {
		if (popcount(OccupiedBB[WHITE][KING]) == 0) return VALUE_MATE;
		if (popcount(OccupiedBB[BLACK][KING]) == 0) return -VALUE_MATE;
		
		Value score = VALUE_ZERO;

		Bitboard wPawns = OccupiedBB[WHITE][PAWN], bPawns = OccupiedBB[BLACK][PAWN];
		Bitboard wKnights = OccupiedBB[WHITE][KNIGHT], bKnights = OccupiedBB[BLACK][KNIGHT];
		Bitboard wBishops = OccupiedBB[WHITE][BISHOP], bBishops = OccupiedBB[BLACK][BISHOP];
		Bitboard wRooks = OccupiedBB[WHITE][ROOK], bRooks = OccupiedBB[BLACK][ROOK];
		Bitboard wQueens = OccupiedBB[WHITE][QUEEN], bQueens = OccupiedBB[BLACK][QUEEN];
		Bitboard wKings = OccupiedBB[WHITE][KING], bKings = OccupiedBB[BLACK][KING];
		Bitboard occ = OccupiedBB[BOTH][ANY_PIECE];

		while (wPawns)
			score += PawnTable[pop_lsb(&wPawns)] + PVAL;

		while (bPawns)
			score -= (PawnTable[Mirror64[pop_lsb(&bPawns)]] + PVAL);

		while (wKnights)
			score += KnightTable[pop_lsb(&wKnights)] + NVAL;

		while (bKnights)
			score -= (KnightTable[Mirror64[pop_lsb(&bKnights)]] + NVAL);

		while (wBishops)
			score += BishopTable[pop_lsb(&wBishops)] + BVAL;

		while (bBishops)
			score -= (BishopTable[Mirror64[pop_lsb(&bBishops)]] + BVAL);

		while (wRooks)
			score += RookTable[pop_lsb(&wRooks)] + RVAL;
		
		while (bRooks)
			score -= (RookTable[Mirror64[pop_lsb(&bRooks)]] + RVAL);

		score += popcount(wQueens) * QVAL;
		score -= popcount(bQueens) * QVAL;

		return (pos.side_to_move() == WHITE ? score : -score);
	}
}