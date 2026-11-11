// =============================================================================
// evaluator.cpp: definitions for include/chess/evaluator.hpp
// =============================================================================
// New C++ ideas: a type alias for a long type name, returning a const
// reference to a big object instead of copying it.

#include "chess/evaluator.hpp"

#include <array>

namespace chess {
namespace {

// PIECE-SQUARE TABLES: for each piece type, a bonus (or penalty) for each of
// the 64 squares. The numbers come from a well-known simple chess-engine
// design ("Simplified Evaluation Function" by Tomasz Michniewski).
//
// `using Table = std::array<int, 64>;` is a TYPE ALIAS (see Square in
// types.hpp): "Table" is now a short name for "a list of exactly 64 ints".
//
// The numbers are laid out the way you SEE the board from White's side:
// the first row of numbers is rank 8 (the far side), the last row is rank 1.
// (The line breaks are just for humans; to C++ it's one list of 64 numbers.)
using Table = std::array<int, 64>;

// Pawns: rewarded for advancing and for controlling the centre.
constexpr Table PawnTable{
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0};

// Knights: strong in the centre, weak on the edge.
constexpr Table KnightTable{
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50};

constexpr Table BishopTable{
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20};

constexpr Table RookTable{
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0};

constexpr Table QueenTable{
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20};

// King: stay safely tucked in the corner behind pawns. (Good early in the
// game; wrong in the endgame, when the king should become active. A known
// weakness, listed in LEARNING.md.)
constexpr Table KingTable{
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20};

// Returns the right table for a piece type.
// The return type `const Table&` is a const REFERENCE: we hand back the
// existing table itself (read-only) instead of a copy of all 64 numbers.
// This is safe only because the tables live for the whole program; never
// return a reference to a local variable, which disappears when the function ends.
constexpr const Table& tableFor(PieceType type) noexcept {
    switch (type) {
        case PieceType::Pawn: return PawnTable;
        case PieceType::Knight: return KnightTable;
        case PieceType::Bishop: return BishopTable;
        case PieceType::Rook: return RookTable;
        case PieceType::Queen: return QueenTable;
        default: return KingTable;
    }
}

// Converts a board square into a position in the tables above.
// The tables are written rank 8 first, so a WHITE piece on rank r is in table
// row (7 - r). For BLACK we flip the board upside down (row r), so the same
// table works for both sides: Black's "good squares" mirror White's.
constexpr std::size_t tableIndex(Square sq, Color color) noexcept {
    const int row = color == Color::White ? 7 - rankOf(sq) : rankOf(sq);
    return static_cast<std::size_t>(row * 8 + fileOf(sq));
}

}  // namespace

int Evaluator::evaluateWhite(const Board& board) const {
    int score = 0;
    for (Square sq = 0; sq < 64; ++sq) {
        const Piece piece = board.pieceAt(sq);
        if (piece.isEmpty()) continue;
        // Piece value + square bonus. tableFor(...) gives a table, and
        // [tableIndex(...)] picks one number from it.
        const int value = pieceValue(piece.type) + tableFor(piece.type)[tableIndex(sq, piece.color)];
        // White's pieces add to the score, Black's subtract.
        score += piece.color == Color::White ? value : -value;
    }
    return score;
}

int Evaluator::evaluate(const Board& board) const {
    const int white = evaluateWhite(board);
    // Flip the sign when it's Black's turn, so "positive = good for the
    // player to move" always holds.
    return board.sideToMove() == Color::White ? white : -white;
}

}  // namespace chess
