#pragma once

#include "position.h"
#include "utils/defs.h"

namespace Movegen {
	void init_mvvlva();

	void get_moves(Position& pos, Movelist& list);
	void get_captures(Position& pos, Movelist& list);
}