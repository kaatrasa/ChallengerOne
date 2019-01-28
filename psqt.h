#pragma once

#include "utils/defs.h"

namespace PSQT {
	extern Value psq[COLOR_NB][PIECETYPE_NB][SQUARE_NB][PHASE_NB];

	void init();
}