// =============================================================================
// evaluator.hpp: scores a position: "who is better here, and by how much?"
// =============================================================================
// Scores are in CENTIPAWNS: 100 = being one pawn ahead. Positive is good.
// The score is material (what pieces you have) plus small bonuses for pieces
// standing on good squares (e.g. knights in the centre).

#pragma once

#include "chess/board.hpp"

namespace chess {

class Evaluator {
public:
    // A static constexpr function: callable as Evaluator::pieceValue(...) with
    // no object, and computable at compile time. (Each word: see types.hpp
    // and board.hpp.)
    [[nodiscard]] static constexpr int pieceValue(PieceType type) noexcept {
        switch (type) {
            case PieceType::Pawn: return 100;
            case PieceType::Knight: return 320;
            case PieceType::Bishop: return 330;
            case PieceType::Rook: return 500;
            case PieceType::Queen: return 900;
            case PieceType::King: return 0;  // can never be captured, so not counted
            case PieceType::None: return 0;
        }
        return 0;  // never reached, but keeps the compiler sure every path returns
    }

    // Normal (non-static) member functions: you need an Evaluator object to
    // call them:   Evaluator eval;   int s = eval.evaluate(board);
    // The object has no data yet, but later improvements may add some (e.g.
    // tuning weights), and code calling it won't need to change.

    // Positive = good for White, no matter whose turn it is.
    [[nodiscard]] int evaluateWhite(const Board& board) const;

    // Positive = good for whoever's turn it is. The search needs this form.
    [[nodiscard]] int evaluate(const Board& board) const;
};

}  // namespace chess
