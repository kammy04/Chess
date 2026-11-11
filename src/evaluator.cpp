#include "chess/evaluator.hpp"

#include <array>

namespace chess {
namespace {

// Piece-square tables from Tomasz Michniewski's "Simplified Evaluation
// Function". Written as you would SEE the board from White's side: the first
// row is rank 8, the last row is rank 1.
using Table = std::array<int, 64>;

constexpr Table PawnTable{
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0};

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

// Middlegame king: stay castled behind pawns. (No endgame table yet -- a
// known weakness, listed in LEARNING.md.)
constexpr Table KingTable{
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20};

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

// Tables are written rank 8 first, so a WHITE piece on rank r reads row (7 - r).
// A BLACK piece uses the vertically mirrored square: row r.
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
        const int value = pieceValue(piece.type) + tableFor(piece.type)[tableIndex(sq, piece.color)];
        score += piece.color == Color::White ? value : -value;
    }
    return score;
}

int Evaluator::evaluate(const Board& board) const {
    const int white = evaluateWhite(board);
    return board.sideToMove() == Color::White ? white : -white;
}

}  // namespace chess
