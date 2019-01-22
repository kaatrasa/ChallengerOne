An UCI compatible chess engine, with following features:

Board representation
Magic Bitboards implemented using PEXT(BMI2).

Search
Alpha-beta pruning with iterative deepening and quiescence search.
Aspiration windows.
Null move pruning.
Simple transposition table.
One thread.

Move Ordering
Captures are ordered by MVV/LVA and quiet moves by history and killer heuristics.

Evaluation
Very basic, only counts material and uses piece square tables.

In future, I'm looking forward to improving evaluation, adding pruning methods to search and making it parallel.

My main sources have been https://www.chessprogramming.org/Main_Page and https://github.com/official-stockfish/Stockfish and I want to thank both for tremendous help.