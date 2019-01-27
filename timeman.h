#pragma once

#include <chrono>

#include "utils/defs.h"

namespace Timeman {
	inline int get_time() {
		return std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	inline void check_time_up(SearchInfo& info) {
		if ((info.nodes & 2047) == 0) {
			if (info.timeSet == true && Timeman::get_time() > info.stopTime) {
				info.stopped = true;
			}
		}
	}
}
