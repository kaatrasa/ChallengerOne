#pragma once

#include "utils/defs.h"

namespace PSQT {
	constexpr Value PieceValue[PHASE_NB][PIECETYPE_NB] = {
		{ VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg },
		{ VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg }
	};
	
	extern Value psq[COLOR_NB][PIECETYPE_NB][SQUARE_NB][PHASE_NB];

	void init();
}