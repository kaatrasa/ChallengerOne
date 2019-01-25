#pragma once

#include <intrin.h>

#define NOMINMAX

typedef unsigned long long Bitboard;
typedef unsigned long long Key;

constexpr int MAX_GAMELENGTH = 256;
constexpr int MAX_POSITIONMOVES = 256;
constexpr int MAX_DEPTH = 64;
constexpr int SQUARE_CNT = 64;
constexpr int FILE_CNT = 8;
constexpr int RANK_CNT = 8;
constexpr int MATE_SCORE = 100000;
constexpr int INF = 2000000;

enum Color { 
	WHITE, BLACK, BOTH 
};

enum Piece { 
	EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK 
};

enum PieceType { 
	NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ANY_PIECE = 0 
};

enum File : int { 
	FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H 
};

enum Rank : int { 
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 
};

enum Square : int {
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	
	SQ_NONE
};

enum Move : int {
	MOVE_NONE
};

enum MoveFlag : int {
	NO_FLAG = 0,
	FLAGEP = 0x200000,
	FLAGPS = 0x400000,
	FLAGCA = 0x8000000,
	FLAGCAP = 0x38000,
	FLAGPROM = 0x1C0000
};

enum CastlingRight : int {
	NO_CASTLING = 0,
	WKCA = 1,
	WQCA = 2,
	BKCA = 4,
	BQCA = 8
};

enum TTFlag { 
	NO_FLAG_TT, EXACT, UPPERBOUND, LOWERBOUND 
};

enum Direction : int {
	NORTH = 8,
	EAST = 1,
	SOUTH = -NORTH,
	WEST = -EAST,

	NORTH_EAST = NORTH + EAST,
	SOUTH_EAST = SOUTH + EAST,
	SOUTH_WEST = SOUTH + WEST,
	NORTH_WEST = NORTH + WEST
};

struct Undo {
	Move move;
	CastlingRight castlePerm;
	Square enPas;
	int fiftyMove;
	Key posKey;
};

struct MoveEntry {
	Move move;
	int score;
};

struct Movelist {
	MoveEntry moves[MAX_POSITIONMOVES];
	int count = 0;
};

struct SearchInfo {
	int startTime;
	int stopTime;
	int depth;
	int depthSet;
	bool timeSet;
	int movestogo;
	int infinite;

	long nodes;

	int quit;
	int stopped;

	float fh;
	float fhf;
};

// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square& operator+=(Square& s, Direction d) { return s = s + d; }
inline Square& operator-=(Square& s, Direction d) { return s = s - d; }

inline CastlingRight& operator &=(CastlingRight& r, int p) { return r = CastlingRight(r & p);  };

constexpr Color operator~(Color c) {
	return Color(c ^ BLACK); // Toggle color
}

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
constexpr T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }         \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
ENABLE_INCR_OPERATORS_ON(T)                                        \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Direction)
ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Piece)
ENABLE_INCR_OPERATORS_ON(Color)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)
ENABLE_BASE_OPERATORS_ON(CastlingRight)

constexpr bool is_ok(Square s) {
	return s >= SQ_A1 && s <= SQ_H8;
}

constexpr File file_of(Square s) {
	return File(s & 7);
}

constexpr Rank rank_of(Square s) {
	return Rank(s >> 3);
}

constexpr Square from_sq(Move m) {
	return Square(m & 0x3F);
}

constexpr Square to_sq(Move m) {
	return Square((m >> 6) & 0x3F);
}

constexpr PieceType captured(Move m) {
	return PieceType((m >> 15) & 0x7);
}

constexpr PieceType promoted(Move m) {
	return PieceType((m >> 18) & 0x7);
}

constexpr Move make(Square f, Square t, PieceType mo, PieceType ca, PieceType pro, MoveFlag fl) {
	return Move(f | (t << 6) | (mo << 12) | (ca << 15) | (pro << 18) | fl);
}