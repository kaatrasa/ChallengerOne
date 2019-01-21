#pragma once

#include "position.h"
#include "utils/defs.h"

struct TTEntry {
	TTFlag flag;
	int depth;
	int score;
	Move move;
	Key posKey;
};

class TranspositionTable {
public:
	TranspositionTable();
	~TranspositionTable() { free(mem_); }

	void clear();
	void save(Key posKey, const Move move, int score, TTFlag flag, const int depth);
	TTEntry* probe(Key key, bool& found) const;

private:
	TTEntry* table_;
	void* mem_;
	unsigned long long entryCount_;

	const unsigned long long size_ = 0x40000000;
};

extern TranspositionTable TT;