#pragma once

#include <windows.h>

#include "utils/defs.h"

namespace Timeman {
	inline int get_time() {
		return GetTickCount();
	}

	inline void check_time_up(SearchInfo *info) {
		if ((info->nodes & 2047) == 0) {
			if (info->timeSet == true && Timeman::get_time() > info->stopTime) {
				info->stopped = true;
			}
		}
	}
}