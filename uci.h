#pragma once

#include "utils/defs.h"
#include "position.h"

namespace UCI {
	void loop();
	void report(Position& pos, SearchInfo& info, Depth depth, Value eval);
	void report_go_finished(Position& pos, SearchInfo& info);
}