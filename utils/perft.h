#pragma once

#include "defs.h"
#include "../position.h"

namespace Perft {
	unsigned long long perft_begin(Position& pos, int depth, bool print);
	void perft_auto(Position& pos, int maxDepth);
}
