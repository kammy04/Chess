#pragma once

#include "chess/board.hpp"

namespace chess {

// Static evaluation: how good is this position, without searching further?
// Scores are in centipawns (100 = one pawn).
//
// Baseline: material + piece-square tables (bonuses for good squares, e.g.
// knights in the centre, pawns advancing). Deliberately simple so Phase 3
// improvements have something to measure against.
class Evaluator {
public:
    [[nodiscard]] static constexpr int pieceValue(PieceType type) noexcept {
        switch (type) {
            case PieceType::Pawn: return 100;
            case PieceType::Knight: return 320;
            case PieceType::Bishop: return 330;
            case PieceType::Rook: return 500;
            case PieceType::Queen: return 900;
            case PieceType::King: return 0;  // never captured; not counted
            case PieceType::None: return 0;
        }
        return 0;
    }

    // Positive = good for White, regardless of whose turn it is.
    [[nodiscard]] int evaluateWhite(const Board& board) const;

    // Positive = good for the side to move. This is what negamax wants.
    [[nodiscard]] int evaluate(const Board& board) const;
};

}  // namespace chess
