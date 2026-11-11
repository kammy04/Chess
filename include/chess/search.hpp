#pragma once

#include <chrono>
#include <cstdint>

#include "chess/board.hpp"
#include "chess/evaluator.hpp"
#include "chess/move.hpp"

namespace chess {

inline constexpr int Infinity = 1'000'000;
// A mate found N plies from the root scores (MateScore - N), so the engine
// prefers faster mates and slower losses.
inline constexpr int MateScore = 100'000;
inline constexpr int MaxMatePly = 1'000;

[[nodiscard]] constexpr bool isMateScore(int score) noexcept {
    return score >= MateScore - MaxMatePly || score <= -MateScore + MaxMatePly;
}

// Mate distance in full moves, positive if the side to move delivers mate.
[[nodiscard]] constexpr int mateInMoves(int score) noexcept {
    const int plies = MateScore - (score > 0 ? score : -score);
    const int moves = (plies + 1) / 2;
    return score > 0 ? moves : -moves;
}

struct SearchResult {
    Move bestMove;           // null move if the side to move has no legal moves
    int score = 0;           // centipawns from the side to move's point of view
    int depth = 0;
    std::uint64_t nodes = 0; // positions visited -- THE metric for Phase 3
    std::chrono::microseconds elapsed{0};
};

// Fixed-depth negamax search. Negamax is minimax written once instead of
// twice: since chess is zero-sum, max(a, b) for me == -min(-a, -b) for you,
// so every node maximises "-score of the child".
class Search {
public:
    enum class Algorithm {
        Minimax,    // visits every node: the correctness reference
        AlphaBeta,  // same answer, prunes branches that cannot matter
    };

    // `explicit` stops accidental conversions like `Search s = Evaluator{};`.
    explicit Search(Algorithm algorithm = Algorithm::AlphaBeta, Evaluator evaluator = {})
        : evaluator_(evaluator), algorithm_(algorithm) {}

    // The board is searched in place (make/unmake) and restored on return.
    [[nodiscard]] SearchResult findBestMove(Board& board, int depth);

private:
    int negamax(Board& board, int depth, int alpha, int beta, int ply);

    Evaluator evaluator_;
    Algorithm algorithm_;
    std::uint64_t nodes_ = 0;
};

}  // namespace chess
