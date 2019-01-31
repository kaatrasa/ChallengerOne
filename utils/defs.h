#pragma once

#include <assert.h>

#ifdef _WIN32
#define NOMINMAX
#include <intrin.h>
#include <nmmintrin.h>
#else
#include <x86intrin.h>
#endif

typedef unsigned long long Bitboard;
typedef unsigned long long Key;

constexpr int MAX_GAMELENGTH = 256;
constexpr int MAX_POSITIONMOVES = 256;
constexpr int MAX_PLY = 128;

enum Color { 
	WHITE, BLACK, BOTH,

	COLOR_NB = 2
};

enum Piece { 
	EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK 
};

enum PieceType { 
	PIECETYPE_NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, 
	
	PIECETYPE_ANY = 0,
	PIECETYPE_NB = 7
};

enum File : int { 
	FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB,

	FILE_SEMIOPEN = 0,
	FILE_OPEN = 1
};

enum Rank : int { 
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
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
	SQ_NONE,

	SQUARE_NB = 64
};

enum Value : int {
	VALUE_ZERO = 0,
	VALUE_DRAW = 0,
	VALUE_KNOWN_WIN = 10000,
	VALUE_MATE = 32000,
	VALUE_INFINITE = 32001,
	VALUE_NONE = 32002,

	VALUE_MATE_IN_MAX_PLY = VALUE_MATE - 2 * MAX_PLY,
	VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,

	PawnValueMg = 136, PawnValueEg = 208,
	KnightValueMg = 782, KnightValueEg = 865,
	BishopValueMg = 830, BishopValueEg = 918,
	RookValueMg = 1289, RookValueEg = 1378,
	QueenValueMg = 2529, QueenValueEg = 2687,

	MidgameLimit = 15258, EndgameLimit = 3915
};

enum Depth : int {
	ONE_PLY = 1,

	DEPTH_ZERO = 0 * ONE_PLY,
	DEPTH_MAX = MAX_PLY * ONE_PLY
};

enum Move : int {
	MOVE_NONE
};

enum Flag : int {
	// Move contains these flags explicitly
	FLAG_NONE = 0,
	FLAG_EP = 0x200000,
	FLAG_PS = 0x400000,
	FLAG_CASTLE = 0x8000000,

	// Move contains these flags implicitly
	FLAG_CAP = 0x38000,
	FLAG_PROM = 0x1C0000
};

enum Order : int {
	ORDER_NONE,
	ORDER_ZERO = 0,

	ORDER_QUIET   = 0,
	ORDER_CAPTURE = 1000000,
	ORDER_EP      = 1000150,
	ORDER_PROM_Q  = 1100000,
	ORDER_PROM_N  = 10000,
	ORDER_PROM_R  = -1,
	ORDER_PROM_B  = -1,
	ORDER_KILLER1 = 900000,
	ORDER_KILLER2 = 800000,
	ORDER_TT      = 2000000
};

enum CastlingRight : int {
	NO_CASTLING = 0,
	WKCA = 1,
	WQCA = 2,
	BKCA = 4,
	BQCA = 8
};

enum Bound {
	BOUND_NONE, BOUND_EXACT, BOUND_UPPER, BOUND_LOWER
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

enum Phase : int {
	PHASE_MID, PHASE_END, PHASE_NB
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
	Order order;
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
	Move bestMove;

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

// Additional operators to add integers to a Value
constexpr Value operator+(Value v, int i) { return Value(int(v) + i); }
constexpr Value operator-(Value v, int i) { return Value(int(v) - i); }
inline Value& operator+=(Value& v, int i) { return v = v + i; }
inline Value& operator-=(Value& v, int i) { return v = v - i; }

// Additional operators to multiply a Value by Depth, used to calculate depth based margins
constexpr Value operator*(Value v, Depth d) { return Value(int(v) * int(d)); }

inline CastlingRight& operator &=(CastlingRight& r, int p) { return r = CastlingRight(r & p);  };

constexpr Color operator~(Color c) {
	return Color(c ^ BLACK); // Toggle color
}

constexpr Square operator~(Square s) {
	return Square(s ^ SQ_A8); // Vertical flip SQ_A1 -> SQ_A8
}

constexpr File operator~(File f) {
	return File(f ^ FILE_H); // Horizontal flip FILE_A -> FILE_H
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

ENABLE_FULL_OPERATORS_ON(Value)
ENABLE_FULL_OPERATORS_ON(Depth)
ENABLE_FULL_OPERATORS_ON(Direction)
ENABLE_FULL_OPERATORS_ON(Order)

ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Piece)
ENABLE_INCR_OPERATORS_ON(Color)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)
ENABLE_INCR_OPERATORS_ON(Phase)
ENABLE_BASE_OPERATORS_ON(CastlingRight)

constexpr bool is_ok(Square s) {
	return s >= SQ_A1 && s <= SQ_H8;
}

constexpr bool is_ok(PieceType pt) {
	return pt == PAWN
		|| pt == KNIGHT
		|| pt == BISHOP
		|| pt == ROOK
		|| pt == QUEEN
		|| pt == KING;
}

constexpr bool is_ok(Flag fl) {
	return fl == FLAG_EP
		|| fl == FLAG_PS
		|| fl == FLAG_CASTLE;
}

constexpr File file_of(Square s) {
	return File(s & 7);
}

constexpr Rank rank_of(Square s) {
	return Rank(s >> 3);
}

constexpr Value mate_in(int ply) {
	return VALUE_MATE - ply;
}

constexpr Value mated_in(int ply) {
	return -VALUE_MATE + ply;
}

constexpr Square make_square(File f, Rank r) {
	return Square((r << 3) + f);
}

constexpr Square from_sq(Move m) {
	return Square(m & 0x3F);
}

constexpr Square to_sq(Move m) {
	return Square((m >> 6) & 0x3F);
}

constexpr PieceType moved_piece(Move m) {
	return PieceType((m >> 12) & 0x7);
}

constexpr PieceType captured_piece(Move m) {
	return PieceType((m >> 15) & 0x7);
}

constexpr PieceType promoted_piece(Move m) {
	return PieceType((m >> 18) & 0x7);
}

constexpr Move make(Square f, Square t, PieceType mo, PieceType ca, PieceType pro, Flag fl) {
	assert(is_ok(f));
	assert(is_ok(t));
	assert(is_ok(mo));
	assert(is_ok(ca) || ca == PIECETYPE_NONE);
	assert(is_ok(pro) || pro == PIECETYPE_NONE);
	assert(is_ok(fl) || fl == FLAG_NONE);

	return Move(f | (t << 6) | (mo << 12) | (ca << 15) | (pro << 18) | fl);
}

#ifdef _WIN64
inline Square lsb(Bitboard b) {
	assert(b);
	unsigned long idx;
	_BitScanForward64(&idx, b);
	return (Square)idx;
}

#else  // unix
inline Square lsb(Bitboard b) {
	assert(b);
	unsigned long idx;
	idx = __builtin_ffsll(b) - 1;
	return (Square)idx;
}
#endif

inline Square pop_lsb(Bitboard* b) {
	const Square s = lsb(*b);
	*b &= *b - 1;
	return s;
}

inline int popcount(Bitboard b) {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	return (int)_mm_popcnt_u64(b);
#else // Assumed gcc or compatible compiler
	return __builtin_popcountll(b);
#endif
}

inline unsigned int pext(Bitboard b, Bitboard m) {
	return unsigned(_pext_u64(b, m));
}