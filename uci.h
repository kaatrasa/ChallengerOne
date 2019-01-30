#pragma once

#include "utils/defs.h"
#include "position.h"

namespace UCI {
	void loop();
	void report(Position& pos, SearchInfo& info, Depth depth, Value eval);
	void report_best_move(Position& pos, SearchInfo& info);
}