// =============================================================================
// search.hpp: the "thinking" part: looks ahead to choose the best move
// =============================================================================
// New C++ ideas: std::chrono (measuring time), a struct used to return
// several results at once, an enum declared inside a class, an `explicit`
// constructor with a member initializer list.
//
// HOW THE SEARCH WORKS (plain English; details in LEARNING.md):
// To pick a move, imagine every move I could make; for each, every reply my
// opponent could make; for each, every move I could make back; and so on, to
// a fixed number of moves ahead (the DEPTH). At the end of each line, score
// the position with the Evaluator. Then work backwards, assuming both players
// always choose their best option. This is MINIMAX. ALPHA-BETA is a smarter
// version that skips lines which provably can't change the answer.

#pragma once

#include <chrono>   // std::chrono: clocks and time durations
#include <cstdint>

#include "chess/board.hpp"
#include "chess/evaluator.hpp"
#include "chess/move.hpp"

namespace chess {

// Score constants. The ' in 1'000'000 is a DIGIT SEPARATOR: it's ignored by
// the compiler and just makes big numbers readable.
inline constexpr int Infinity = 1'000'000;  // bigger than any real score

// Checkmate scores. Being checkmated N half-moves ("plies") from now scores
// -(MateScore - N); delivering mate scores +(MateScore - N). Subtracting N
// makes the engine prefer FASTER wins and SLOWER losses.
inline constexpr int MateScore = 100'000;
inline constexpr int MaxMatePly = 1'000;

// Is this score a "mate found" score rather than a normal evaluation?
[[nodiscard]] constexpr bool isMateScore(int score) noexcept {
    return score >= MateScore - MaxMatePly || score <= -MateScore + MaxMatePly;
}

// Turns a mate score into "mate in N moves" (negative if WE are being mated).
// One full move = two plies (one by each player), hence (plies + 1) / 2.
[[nodiscard]] constexpr int mateInMoves(int score) noexcept {
    const int plies = MateScore - (score > 0 ? score : -score);
    const int moves = (plies + 1) / 2;
    return score > 0 ? moves : -moves;
}

// Everything the search reports back, bundled in one struct so a function can
// return several values at once.
struct SearchResult {
    Move bestMove;            // the null move (Move{}) if there are no legal moves
    int score = 0;            // in centipawns, from the moving player's point of view
    int depth = 0;
    std::uint64_t nodes = 0;  // how many positions were visited: THE number for measuring speed-ups
    // A time length in microseconds (millionths of a second); {0} = zero.
    std::chrono::microseconds elapsed{0};
};

class Search {
public:
    // An enum class declared INSIDE the class: its full name is
    // Search::Algorithm, so values are written Search::Algorithm::Minimax.
    enum class Algorithm {
        Minimax,    // looks at everything: slow, but obviously correct
        AlphaBeta,  // same answer, skips branches that can't matter
    };

    // CONSTRUCTOR with DEFAULT ARGUMENTS (see Board::render), so all of these work:
    //   Search s;                                   // AlphaBeta, default Evaluator
    //   Search s(Search::Algorithm::Minimax);       // choose the algorithm
    // `explicit` blocks accidental automatic conversions. Without it, C++
    // would let you write `Search s = Search::Algorithm::Minimax;`, quietly
    // turning an Algorithm into a Search. Rule of thumb: mark one-argument
    // constructors explicit.
    // `Evaluator evaluator = {}` means "a default Evaluator if none is given".
    //
    // The part after the colon, `: evaluator_(evaluator), algorithm_(algorithm)`,
    // is the MEMBER INITIALIZER LIST: it sets each member variable directly
    // as the object is created. That is the preferred way to fill members in
    // a constructor. The { } after it is the (empty) constructor body.
    explicit Search(Algorithm algorithm = Algorithm::AlphaBeta, Evaluator evaluator = {})
        : evaluator_(evaluator), algorithm_(algorithm) {}

    // Searches `depth` plies ahead and returns the best move and statistics.
    // Takes Board& (not const): it makes and unmakes moves on the board while
    // thinking, and leaves it exactly as it was when done.
    [[nodiscard]] SearchResult findBestMove(Board& board, int depth);

private:
    // The recursive helper that does the real work (see search.cpp).
    int negamax(Board& board, int depth, int alpha, int beta, int ply);

    // Member variables (trailing _ by convention, see board.hpp).
    Evaluator evaluator_;
    Algorithm algorithm_;
    std::uint64_t nodes_ = 0;  // counter, reset at the start of each search
};

}  // namespace chess
