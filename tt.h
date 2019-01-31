#pragma once

#include "position.h"
#include "utils/defs.h"

struct TTEntry {
	Bound bound;
	Depth depth;
	Value value;
	Move move;
	Key posKey;
};

class TranspositionTable {
public:
	TranspositionTable();
	~TranspositionTable() { free(mem_); }

	void clear();
	void save(const Key posKey, const Move move, const Value value, const Bound bound, const Depth depth);
	TTEntry* probe(Key key, bool& found) const;

private:
	TTEntry* table_;
	void* mem_;
	unsigned long long entryCount_;

	const unsigned long long size_ = 0x4000000;
};

extern TranspositionTable TT;