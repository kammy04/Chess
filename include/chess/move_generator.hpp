// =============================================================================
// move_generator.hpp: lists every move the player to move can make
// =============================================================================
// New C++ ideas: a class with only static member functions, returning a
// std::vector, std::pair, and std::uint64_t (a very large whole number).

#pragma once

#include <cstdint>
#include <utility>  // std::pair: two values bundled together
#include <vector>

#include "chess/board.hpp"
#include "chess/move.hpp"

namespace chess {

// Every function here is `static` (see Board::fromFen in board.hpp): you call
// them as MoveGenerator::generateLegal(board), with no MoveGenerator object
// needed, because the generator has no data of its own to remember.
// It's still a class so a future, faster version can store precomputed
// tables inside it without changing how anyone calls it.
class MoveGenerator {
public:
    // "Pseudo-legal" moves follow how each piece moves, but might leave your
    // own king in check (which is illegal). Faster; used as a first step.
    //   std::vector<Move>   returns a list of Moves (any length)
    //   const Board& board  receives the board by const reference: read-only
    [[nodiscard]] static std::vector<Move> generatePseudoLegal(const Board& board);

    // Only fully legal moves. Note `Board& board` WITHOUT const: this
    // function DOES change the board temporarily. It tries each move
    // (makeMove), checks whether the king is safe, then takes it back
    // (unmakeMove). When it returns, the board is exactly as it was.
    [[nodiscard]] static std::vector<Move> generateLegal(Board& board);

    // PERFT ("performance test"): counts every possible sequence of `depth`
    // moves from this position. Chess programmers have published the correct
    // counts for standard positions, so matching them proves the move
    // generator is right. (See LEARNING.md.)
    // std::uint64_t is an unsigned 64-bit integer: holds numbers up to about
    // 18 quintillion. The counts get big fast.
    [[nodiscard]] static std::uint64_t perft(Board& board, int depth);

    // Perft broken down per first move: a list of (move, count) pairs.
    // std::pair<A, B> holds two values; you read them as .first and .second.
    // Used to find WHICH move is wrong if a perft count ever disagrees.
    [[nodiscard]] static std::vector<std::pair<Move, std::uint64_t>> perftDivide(Board& board, int depth);
};

}  // namespace chess
