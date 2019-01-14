#pragma once

#include "position.h"

enum PieceValue : int {
	PVAL = 100,
	NVAL = 300,
	BVAL = 320,
	RVAL = 500,
	QVAL = 900,
	KVAL = 100000
};

namespace Evaluation {
	int evaluate(const Position& pos);
}