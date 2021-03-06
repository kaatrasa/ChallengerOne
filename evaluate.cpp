#include <algorithm>
#include <iostream>

#include "evaluate.h"
#include "utils/defs.h"

namespace Evaluation {

	constexpr Value LazyThreshold = Value(1500);
	constexpr Value Tempo = Value(28);

	// MobilityBonus[PieceType-2][attacked] contains bonuses for middle and end game,
	// indexed by piece type and number of attacked squares in the mobility area.
	constexpr Value MobilityBonus[][32][PHASE_NB] = {
		{
			// Knight
			{Value(-62), Value(-81) },
			{Value(-53), Value(-56) },
			{Value(-12), Value(-30) },
			{Value(-4),  Value(-14) },
			{Value(3),   Value(8)   },
			{Value(13),  Value(15)  },
			{Value(22),  Value(23)  },
			{Value(28),  Value(27)  },
			{Value(33),  Value(33)  }
		},
		{
			// Bishop
			{ Value(-48), Value(-59) },
			{ Value(-20), Value(-23) },
			{ Value(16),  Value(-3)	 },
			{ Value(26),  Value(13)	 },
			{ Value(38),  Value(24)	 },
			{ Value(51),  Value(42)	 },
			{ Value(55),  Value(54)	 },
			{ Value(63),  Value(57)	 },
			{ Value(63),  Value(65)	 },
			{ Value(68),  Value(73)	 },
			{ Value(81),  Value(78)	 },
			{ Value(81),  Value(86)	 },
			{ Value(91),  Value(88)	 },
			{ Value(98),  Value(97)	 }
		},								 
		{
			// Rook
			{ Value(-58), Value(-76) },
			{ Value(-27), Value(-18) },
			{ Value(-15), Value(28)  },
			{ Value(-10), Value(55)  },
			{ Value(-5),  Value(69)  },
			{ Value(-2),  Value(82)  },
			{ Value(9),   Value(112) },
			{ Value(16),  Value(118) },
			{ Value(30),  Value(132) },
			{ Value(29),  Value(142) },
			{ Value(32),  Value(155) },
			{ Value(38),  Value(165) },
			{ Value(46),  Value(166) },
			{ Value(48),  Value(169) },
			{ Value(58),  Value(171) }
		},
		{
			// Queen
			{ Value(-39), Value(-36) },
			{ Value(-21), Value(-15) },
			{ Value(3),   Value(8)   },
			{ Value(3),   Value(18)  },
			{ Value(14),  Value(34)  },
			{ Value(22),  Value(54)  },
			{ Value(28),  Value(61)  },
			{ Value(41),  Value(73)  },
			{ Value(43),  Value(79)  },
			{ Value(48),  Value(92)  },
			{ Value(56),  Value(94)  },
			{ Value(60),  Value(104) },
			{ Value(60),  Value(113) },
			{ Value(66),  Value(120) },
			{ Value(67),  Value(123) },
			{ Value(70),  Value(126) },
			{ Value(71),  Value(133) },
			{ Value(73),  Value(136) },
			{ Value(79),  Value(140) },
			{ Value(88),  Value(143) },
			{ Value(88),  Value(148) },
			{ Value(99),  Value(166) },
			{ Value(102), Value(170) },
			{ Value(102), Value(175) },
			{ Value(106), Value(184) },
			{ Value(109), Value(191) },
			{ Value(113), Value(206) },
			{ Value(116), Value(212) },
		}
	};

	// Outpost[knight/bishop][supported by pawn] contains bonuses for minor
	// pieces if they occupy or can reach an outpost square, bigger if that
	// square is supported by a pawn.
	constexpr Value Outpost[2][2][PHASE_NB] = {
		{   
			// Knight
			{ Value(22), Value(6)  },
			{ Value(36), Value(12) }
		},
		{   
			// Bishop
			{ Value(9), Value(2)  },
			{ Value(15), Value(5) }
		}
	};

	// RookOnFile[semiopen/open] contains bonuses for each rook when there is
	// no (friendly) pawn on the rook file.
	constexpr Value RookOnFile[][PHASE_NB] = { 
		{ Value(18), Value(7)  },
		{ Value(44), Value(20) }
	};

	// ThreatByMinor/ByRook[attacked PieceType] contains bonuses according to
	// which piece type attacks which one. Attacks on lesser pieces which are
	// pawn-defended are not considered.
	constexpr Value ThreatByMinor[PIECETYPE_NB][PHASE_NB] = {
		{ Value(0),  Value(0)   },
		{ Value(0),  Value(31)  },
		{ Value(39), Value(42)  },
		{ Value(57), Value(44)  },
		{ Value(68), Value(112) },
		{ Value(62), Value(120) }
	};

	constexpr Value ThreatByRook[PIECETYPE_NB][PHASE_NB] = {
		{ Value(0),  Value(0)  },
		{ Value(0),  Value(24) },
		{ Value(38), Value(71) },
		{ Value(38), Value(61) },
		{ Value(0),  Value(38) },
		{ Value(51), Value(38) }
	};

	// PassedRank[Rank] contains a bonus according to the rank of a passed pawn
	constexpr Value PassedRank[RANK_NB][PHASE_NB] = {
		{ Value(0),   Value(0)   },
		{ Value(5),   Value(18)  },
		{ Value(12),  Value(23)  },
		{ Value(10),  Value(31)  },
		{ Value(57),  Value(62)  },
		{ Value(163), Value(167) },
		{ Value(271), Value(250) }
	};

	// PassedFile[File] contains a bonus according to the file of a passed pawn
	constexpr Value PassedFile[FILE_NB][PHASE_NB] = {
		{ Value(-1),  Value(7)   },
		{ Value(0),   Value(9)   },
		{ Value(-9),  Value(-8)  },
		{ Value(-30), Value(-14) },
		{ Value(-30), Value(-14) },
		{ Value(-9),  Value(-8)  },
		{ Value(0),   Value(9)   },
		{ Value(-1),  Value(7)   }
	};

	// kingRing[color] are the squares adjacent to the king, plus (only for a
	// king on its first rank) the squares two ranks in front. For instance,
	// if black's king is on g8, kingRing[BLACK] is f8, h8, f7, g7, h7, f6, g6
	// and h6.
	Bitboard kingRing[COLOR_NB];

	// kingAttackersCount[color] is the number of pieces of the given color
	// which attack a square in the kingRing of the enemy king.
	int kingAttackersCount[COLOR_NB];

	// kingAttackersWeight[color] is the sum of the "weights" of the pieces of
	// the given color which attack a square in the kingRing of the enemy king.
	// The weights of the individual piece types are given by the elements in
	// the KingAttackWeights array.
	int kingAttackersWeight[COLOR_NB];

	Bitboard MobilityArea[COLOR_NB];
	Value Mobility[COLOR_NB] = { VALUE_ZERO, VALUE_ZERO };

	// Evaluation::initialize() computes king and pawn attacks, and the king ring
	// bitboard for a given color. This is done at the beginning of the evaluation.
	template<Color Us>
	void initialize() {
		constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
		constexpr Direction Up = (Us == WHITE ? NORTH : SOUTH);
		constexpr Direction Down = (Us == WHITE ? SOUTH : NORTH);
		constexpr Bitboard LowRanks = (Us == WHITE ? Rank2BB | Rank3BB : Rank7BB | Rank6BB);

		// Find our pawns that are blocked or on the first two ranks
		Bitboard b = OccupiedBB[Us][PAWN] & (shift<Down>(OccupiedBB[BOTH][PIECETYPE_ANY]) | LowRanks);

		// Squares occupied by those pawns, by our king or queen, or controlled by enemy pawns
		// are excluded from the mobility area.
		MobilityArea[Us] = ~(b | OccupiedBB[Us][KING] | OccupiedBB[Us][QUEEN] | pawn_attacks_bb<Them>(OccupiedBB[Them][PAWN]));
		Mobility[Us] = VALUE_ZERO;
	}

	// Evaluation::pieces() scores pieces of a given color and type
	template<Color Us, PieceType Pt>
	Value pieces(const Position& pos, Phase ph) {
		Bitboard bb = pos.pieces(Us, Pt);
		Value value = VALUE_ZERO;
		Square s;

		while (bb) 
		{
			s = pop_lsb(&bb);

			// Find attacked squares, including x-ray attacks for bishops and rooks
			Bitboard attacks = Pt == BISHOP ? attacks_bb<BISHOP>(s, pos.pieces() ^ pos.pieces(QUEEN))
							 : Pt == ROOK   ? attacks_bb<  ROOK>(s, pos.pieces() ^ pos.pieces(QUEEN) ^ pos.pieces(Us, ROOK))
										    : pos.attacks_from<Pt>(s);

			int mob = popcount(attacks & MobilityArea[Us]);
			Mobility[Us] += MobilityBonus[Pt - 2][mob][ph];
		}
		
		return value;
	}

	Value evaluate(const Position& pos) {
		if (popcount(OccupiedBB[WHITE][KING]) == 0) return VALUE_MATE;
		if (popcount(OccupiedBB[BLACK][KING]) == 0) return -VALUE_MATE;

		Color us = pos.side_to_move();
		Color them = ~us;
		Value valueMg = pos.psq_score(PHASE_MID);
		Value valueEg = pos.psq_score(PHASE_END);
		Value value = (valueMg + valueEg) / 2;
		Phase ph = pos.non_pawn_material() <= EndgameLimit ? PHASE_END : PHASE_MID;

		// Early exit if score is high
		if (abs(value) > LazyThreshold)
			return us == WHITE ? value : -value;

		initialize<WHITE>();
		initialize<BLACK>();
		
		// Pieces should be evaluated first (populate attack tables)
		value += pieces<WHITE, KNIGHT>(pos, ph) - pieces<BLACK, KNIGHT>(pos, ph)
			  +  pieces<WHITE, BISHOP>(pos, ph) - pieces<BLACK, BISHOP>(pos, ph)
			  +  pieces<WHITE, ROOK  >(pos, ph) - pieces<BLACK, ROOK  >(pos, ph)
			  +  pieces<WHITE, QUEEN >(pos, ph) - pieces<BLACK, QUEEN >(pos, ph);
		
		value += Mobility[WHITE] - Mobility[BLACK];

		return (us == WHITE ? value : -value) + Evaluation::Tempo;
	}
}