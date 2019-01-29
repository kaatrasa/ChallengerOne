#pragma once

#include "position.h"
#include "utils/defs.h"

namespace Search {
	enum NodeType { NonPV, PV };

	constexpr Value FutilityMargin = Value(95);
	constexpr Depth FutilityPruningDepth = Depth(8);

	constexpr Value RazorMargin = Value(350);
	constexpr Depth RazorDepth = Depth(1);
	
	constexpr Value BetaMargin = Value(85);
	constexpr Depth BetaPruningDepth = Depth(8);

	constexpr Depth NullMovePruningDepth = Depth(2);

	void start(Position& pos, SearchInfo& info);
}