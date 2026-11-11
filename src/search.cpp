// =============================================================================
// search.cpp: definitions for include/chess/search.hpp
// =============================================================================
// New C++ ideas: std::max, timing code with std::chrono, and recursion used
// for game-tree search.

#include "chess/search.hpp"

#include <algorithm>  // std::max and other general-purpose helpers

#include "chess/move_generator.hpp"

namespace chess {

SearchResult Search::findBestMove(Board& board, int depth) {
    // Read the clock now; we read it again at the end and subtract.
    // steady_clock is a clock that never jumps (unlike wall-clock time).
    // `auto` saves writing the long type name of a time point.
    const auto start = std::chrono::steady_clock::now();
    nodes_ = 1;                   // count the starting position itself
    depth = std::max(depth, 1);   // std::max(a, b) = the larger one, so depth is at least 1

    SearchResult result;          // all fields start at their defaults
    result.depth = depth;

    const std::vector<Move> moves = MoveGenerator::generateLegal(board);
    // .empty() is true when the list has no elements.
    if (moves.empty()) {
        // No legal moves: checkmate if in check, otherwise stalemate (a draw, 0).
        result.score = board.inCheck() ? -MateScore : 0;
    } else {
        int alpha = -Infinity;
        const int beta = Infinity;
        result.score = -Infinity;
        for (const Move& move : moves) {
            board.makeMove(move);
            // The opponent's best score is the negative of ours (zero-sum game),
            // hence the minus sign; see negamax below. Note -beta, -alpha: the
            // window flips too.
            const int score = -negamax(board, depth - 1, -beta, -alpha, 1);
            board.unmakeMove();
            // Keep this move if it's strictly better than the best so far.
            // Using > (not >=) keeps the FIRST of several equally good moves,
            // so results are repeatable.
            if (score > result.score) {
                result.score = score;
                result.bestMove = move;
            }
            if (algorithm_ == Algorithm::AlphaBeta) {
                alpha = std::max(alpha, score);
            }
        }
    }

    result.nodes = nodes_;
    // now() - start is a duration; duration_cast converts it to microseconds.
    // (The angle brackets name the type to convert to: a template argument,
    // like std::optional<Square>.)
    result.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    return result;
}

// NEGAMAX with ALPHA-BETA. Returns how good `board` is for the player to move.
//
// Negamax is minimax written once instead of twice. In a zero-sum game my
// score is minus my opponent's score, so "pick the child that's best for me"
// = "pick the child whose score (from the opponent's view) is LOWEST"
// = "maximise MINUS the child's score". Hence  -negamax(...)  everywhere.
//
// Alpha-beta keeps a WINDOW (alpha, beta):
//   alpha = the score I'm already guaranteed by some other move I've seen
//   beta  = the score my opponent is already guaranteed elsewhere; if this
//           position turns out better than beta for me, the opponent
//           would simply never let us get here
// So once I find a move scoring >= beta, there's no point looking at my other
// moves here: a "beta cutoff". Same answer as minimax, far fewer positions.
//
// `ply` = how many moves deep we are from the root (used for mate distance).
int Search::negamax(Board& board, int depth, int alpha, int beta, int ply) {
    ++nodes_;
    if (depth == 0) {
        // End of the look-ahead: score the position as it stands.
        return evaluator_.evaluate(board);
    }

    const std::vector<Move> moves = MoveGenerator::generateLegal(board);
    if (moves.empty()) {
        // Checkmate (bad for us; less bad the further away it is) or stalemate (0).
        return board.inCheck() ? -MateScore + ply : 0;
    }
    if (board.halfmoveClock() >= 100) {
        return 0;  // draw by the fifty-move rule (checkmate above takes priority)
    }

    int best = -Infinity;
    for (const Move& move : moves) {
        board.makeMove(move);
        // RECURSION: the same function, one ply deeper, from the opponent's
        // point of view (hence the minus signs and the flipped window).
        const int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        board.unmakeMove();

        best = std::max(best, score);
        if (algorithm_ == Algorithm::AlphaBeta) {
            alpha = std::max(alpha, best);
            if (alpha >= beta) {
                break;  // beta cutoff: stop looking at the remaining moves
            }
        }
    }
    return best;
}

}  // namespace chess
