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
		entry->depth = DEPTH_ZERO;
		entry->value = VALUE_NONE;
		entry->flag = NO_FLAG_TT;
	}
}

void TranspositionTable::save(const Key posKey, const Move move, const Value value, const TTFlag flag, const Depth depth) {
	unsigned long long index = posKey % entryCount_;

	TTEntry* replace = &table_[index];

	// Don't overwrite an entry from the same position, unless we have
	// an exact bound or depth that is nearly as good as the old one
	if (replace->flag != NO_FLAG_TT
		|| (flag != EXACT
			&&  replace->posKey == posKey
			&& depth < replace->depth - 3))
		return;

	table_[index].move = move;
	table_[index].posKey = posKey;
	table_[index].flag = flag;
	table_[index].depth = depth;
	table_[index].value = value;
}

TTEntry* TranspositionTable::probe(Key key, bool& found) const {
	unsigned long long index = key % entryCount_;

	if (table_[index].posKey == key)
		return found = true, &table_[index];

	return found = false, &table_[index];
}