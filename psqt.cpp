#include <algorithm>

#include "psqt.h"

namespace PSQT {

	// Bonus[PieceType][Square / 2] contains Piece-Square scores. For each piece
	// type on a given square a (middlegame, endgame) score pair is assigned. Table
	// is defined for files A..D and white side: it is symmetric for black side and
	// second half of the files.
	constexpr Value Bonus[][RANK_NB][int(FILE_NB) / 2][PHASE_NB] = {
		{},
		{},
		{   // Knight
			{ { Value(-169), Value(-105) },{ Value(-96), Value(-74) },{ Value(-80), Value(-46) },{ Value(-79), Value(-18) } },
			{ { Value(-79),  Value(-70) },{ Value(-39), Value(-56) },{ Value(-24), Value(-15) },{ Value(-9),  Value(6) } },
			{ { Value(-64),  Value(-38) },{ Value(-20), Value(-33) },{ Value(4),   Value(-5) },{ Value(19),  Value(27) } },
			{ { Value(-28),  Value(-36) },{ Value(5),   Value(0) },{ Value(41),  Value(13) },{ Value(47),  Value(34) } },
			{ { Value(-29),  Value(-41) },{ Value(13),  Value(-20) },{ Value(42),  Value(4) },{ Value(52),  Value(35) } },
			{ { Value(-11),  Value(-51) },{ Value(28),  Value(-38) },{ Value(63),  Value(-17) },{ Value(55),  Value(19) } },
			{ { Value(-67),  Value(-64) },{ Value(-21), Value(-45) },{ Value(6),   Value(-37) },{ Value(37),  Value(16) } },
			{ { Value(-200), Value(-98) },{ Value(-80), Value(-89) },{ Value(-53), Value(-53) },{ Value(-32), Value(-16) } }
		},
		{   // Bishop
			{ { Value(-44), Value(-63) },{ Value(-4),  Value(-30) },{ Value(-11), Value(-35) },{ Value(-28), Value(-8) } },
			{ { Value(-18), Value(-38) },{ Value(7),   Value(-13) },{ Value(14),  Value(-14) },{ Value(3),   Value(0) } },
			{ { Value(-8),  Value(-18) },{ Value(24),  Value(0) },{ Value(-3),  Value(-7) },{ Value(15),  Value(13) } },
			{ { Value(1),   Value(-26) },{ Value(8),   Value(-3) },{ Value(26),  Value(1) },{ Value(37),  Value(16) } },
			{ { Value(-7),  Value(-24) },{ Value(30),  Value(-6) },{ Value(23),  Value(-10) },{ Value(28),  Value(17) } },
			{ { Value(-17), Value(-26) },{ Value(4),   Value(2) },{ Value(-1),  Value(1) },{ Value(8),   Value(16) } },
			{ { Value(-21), Value(-34) },{ Value(-19), Value(-18) },{ Value(10),  Value(-7) },{ Value(-6),  Value(9) } },
			{ { Value(-48), Value(-51) },{ Value(-3),  Value(-40) },{ Value(-12), Value(-39) },{ Value(-25), Value(-20) } }
		},
		{	// Rook
			{ { Value(-24), Value(-2) },{ Value(-13), Value(-6) },{ Value(-7), Value(-3) },{ Value(2),  Value(-2) } },
			{ { Value(-18), Value(-10) },{ Value(-10), Value(-7) },{ Value(-5), Value(1) },{ Value(9),  Value(0) } },
			{ { Value(-21), Value(10) },{ Value(-7),  Value(-4) },{ Value(3),  Value(2) },{ Value(-1), Value(-2) } },
			{ { Value(-13), Value(-5) },{ Value(-5),  Value(2) },{ Value(-4), Value(-8) },{ Value(-6), Value(8) } },
			{ { Value(-24), Value(-8) },{ Value(-12), Value(5) },{ Value(-1), Value(4) },{ Value(6),  Value(-9) } },
			{ { Value(-24), Value(3) },{ Value(-4),  Value(-2) },{ Value(4),  Value(-10) },{ Value(10), Value(7) } },
			{ { Value(-8),  Value(1) },{ Value(6),   Value(2) },{ Value(10), Value(17) },{ Value(12), Value(-8) } },
			{ { Value(-22), Value(12) },{ Value(-24), Value(-6) },{ Value(-6), Value(13) },{ Value(4),  Value(7) } }
		},
		{	// Queen
			{ { Value(3),  Value(-69) },{ Value(-5), Value(-57) },{ Value(-5), Value(-47) },{ Value(4),  Value(-26) } },
			{ { Value(-3), Value(-55) },{ Value(5),  Value(-31) },{ Value(8),  Value(-22) },{ Value(12), Value(-4) } },
			{ { Value(-3), Value(-39) },{ Value(6),  Value(-18) },{ Value(13), Value(-9) },{ Value(7),  Value(3) } },
			{ { Value(4),  Value(-23) },{ Value(5),  Value(-3) },{ Value(9),  Value(13) },{ Value(8),  Value(24) } },
			{ { Value(0),  Value(-29) },{ Value(14), Value(-6) },{ Value(12), Value(9) },{ Value(5),  Value(21) } },
			{ { Value(-4), Value(-38) },{ Value(10), Value(-18) },{ Value(6),  Value(-12) },{ Value(8),  Value(1) } },
			{ { Value(-5), Value(-50) },{ Value(6),  Value(-27) },{ Value(10), Value(-24) },{ Value(8),  Value(-8) } },
			{ { Value(-2), Value(-75) },{ Value(-2), Value(-52) },{ Value(1),  Value(-43) },{ Value(-2), Value(-36) } }
		},
		{	// King
			{ { Value(272), Value(0) },{ Value(325), Value(41) },{ Value(273), Value(80) },{ Value(190), Value(93) } },
			{ { Value(277), Value(57) },{ Value(305), Value(98) },{ Value(241), Value(138) },{ Value(183), Value(131) } },
			{ { Value(198), Value(86) },{ Value(253), Value(138) },{ Value(168), Value(165) },{ Value(120), Value(173) } },
			{ { Value(169), Value(103) },{ Value(191), Value(152) },{ Value(136), Value(168) },{ Value(108), Value(169) } },
			{ { Value(145), Value(98) },{ Value(176), Value(166) },{ Value(112), Value(197) },{ Value(69),  Value(194) } },
			{ { Value(122), Value(87) },{ Value(159), Value(164) },{ Value(85),  Value(174) },{ Value(36),  Value(189) } },
			{ { Value(87),  Value(40) },{ Value(120), Value(99) },{ Value(64),  Value(128) },{ Value(25),  Value(141) } },
			{ { Value(64),  Value(5) },{ Value(87),  Value(60) },{ Value(49),  Value(75) },{ Value(0),   Value(75) } }
		}
	};

	constexpr Value PBonus[RANK_NB][FILE_NB][PHASE_NB] =
	{	// Pawn (asymmetric distribution)
		{},
		{ { Value(0), Value(-10) },{ Value(-5), Value(-3) },{ Value(10), Value(7) },{ Value(13), Value(-1) },{ Value(21), Value(7) },{ Value(17), Value(6) },{ Value(6), Value(1) },{ Value(-3), Value(-20) } },
		{ { Value(-11), Value(-6) },{ Value(-10), Value(-6) },{ Value(15), Value(-1) },{ Value(22), Value(-1) },{ Value(26), Value(-1) },{ Value(28), Value(2) },{ Value(4), Value(-2) },{ Value(-24), Value(-5) } },
		{ { Value(-9), Value(4) },{ Value(-18), Value(-5) },{ Value(8), Value(-4) },{ Value(22), Value(-5) },{ Value(33), Value(-6) },{ Value(25), Value(-13) },{ Value(-4), Value(-3) },{ Value(-16), Value(-7) } },
		{ { Value(6), Value(18) },{ Value(-3), Value(2) },{ Value(-10), Value(2) },{ Value(1), Value(-9) },{ Value(12), Value(-13) },{ Value(6), Value(-8) },{ Value(-12), Value(11) },{ Value(1), Value(9) } },
		{ { Value(-6), Value(25) },{ Value(-8), Value(17) },{ Value(5), Value(19) },{ Value(11), Value(29) },{ Value(-14), Value(29) },{ Value(0), Value(8) },{ Value(-12), Value(4) },{ Value(-14), Value(12) } },
		{ { Value(-10), Value(-1) },{ Value(6), Value(-6) },{ Value(-5), Value(18) },{ Value(-11), Value(22) },{ Value(-2), Value(22) },{ Value(-14), Value(17) },{ Value(12), Value(2) },{ Value(-1), Value(9) } }
	};

	Value psq[COLOR_NB][PIECETYPE_NB][SQUARE_NB][PHASE_NB];

	// init() initializes piece-square tables: the white halves of the tables are
	// copied from Bonus[] adding the piece value, then the black halves of the
	// tables are initialized by flipping and changing the sign of the white scores.
	void init() {
		for (PieceType pt = PIECETYPE_NONE; pt <= KING; ++pt) {
			for (Square s = SQ_A1; s <= SQ_H8; ++s) {
				File f = std::min(file_of(s), ~file_of(s));

				psq[WHITE][pt][s][PHASE_MID] = PieceValue[PHASE_MID][pt] + (pt == PAWN ? PBonus[rank_of(s)][file_of(s)][PHASE_MID]
					: Bonus[pt][rank_of(s)][f][PHASE_MID]);

				psq[WHITE][pt][s][PHASE_END] = PieceValue[PHASE_END][pt] + (pt == PAWN ? PBonus[rank_of(s)][file_of(s)][PHASE_END]
					: Bonus[pt][rank_of(s)][f][PHASE_END]);

				psq[BLACK][pt][~s][PHASE_MID] = -psq[WHITE][pt][s][PHASE_MID];
				psq[BLACK][pt][~s][PHASE_END] = -psq[WHITE][pt][s][PHASE_END];
			}
		}
	}

}
