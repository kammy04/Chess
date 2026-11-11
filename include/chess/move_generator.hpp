#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "chess/board.hpp"
#include "chess/move.hpp"

namespace chess {

// Generates moves for the side to move.
//
// All functions are static: the generator has no state of its own yet. It is
// still a class (rather than free functions in a namespace) so that Phase 3's
// bitboard version can hold precomputed attack tables as members.
class MoveGenerator {
public:
    // Moves that obey piece movement rules but may leave the mover's own king
    // in check. Cheap; useful as a building block.
    [[nodiscard]] static std::vector<Move> generatePseudoLegal(const Board& board);

    // Fully legal moves. Takes a NON-const reference because it tests each
    // move by making and unmaking it; the board is unchanged on return.
    [[nodiscard]] static std::vector<Move> generateLegal(Board& board);

    // Perft ("performance test"): counts leaf nodes of the legal move tree to
    // `depth`. Comparing against published counts is the standard way to prove
    // a move generator correct.
    [[nodiscard]] static std::uint64_t perft(Board& board, int depth);

    // Perft split by root move -- the tool for finding WHICH move is wrong
    // when a perft count disagrees with the reference.
    [[nodiscard]] static std::vector<std::pair<Move, std::uint64_t>> perftDivide(Board& board, int depth);
};

}  // namespace chess
