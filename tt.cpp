#include "tt.h"

TranspositionTable TT;

TranspositionTable::TranspositionTable() {
	entryCount_ = size_ / sizeof(TTEntry);
	entryCount_ -= 2;
	table_ = (TTEntry *)malloc(entryCount_ * sizeof(TTEntry));
	clear();
}

void TranspositionTable::clear() {
	TTEntry* entry;

	for (entry = table_; entry < table_ + entryCount_; entry++) {
		entry->posKey = 0ULL;
		entry->move = MOVE_NONE;
		entry->depth = 0;
		entry->score = 0;
		entry->flag = NO_FLAG_TT;
	}
}

void TranspositionTable::save(Key posKey, const Move move, int score, TTFlag flag, const int depth) {
	unsigned long long index = posKey % entryCount_;

	table_[index].move = move;
	table_[index].posKey = posKey;
	table_[index].flag = flag;
	table_[index].depth = depth;
	table_[index].score = score;
}

bool TranspositionTable::probe(Key posKey, Move* move, int* score, int alpha, int beta, int depth) const {
	unsigned long long index = posKey % entryCount_;

	if (table_[index].posKey == posKey) {
		*move = table_[index].move;
		if (table_[index].depth >= depth) {
			*score = table_[index].score;

			switch (table_[index].flag) {
			case UPPERBOUND:
				if (*score <= alpha) {
					*score = alpha;
					return true;
				}
				break;
			case LOWERBOUND:
				if (*score >= beta) {
					*score = beta;
					return true;
				}
				break;
			case EXACT:
				return true;
			default:
				break;
			}
		}
	}

	return false;
}