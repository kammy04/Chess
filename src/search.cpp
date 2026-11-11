#include "chess/search.hpp"

#include <algorithm>

#include "chess/move_generator.hpp"

namespace chess {

SearchResult Search::findBestMove(Board& board, int depth) {
    const auto start = std::chrono::steady_clock::now();
    nodes_ = 1;  // the root
    depth = std::max(depth, 1);

    SearchResult result;
    result.depth = depth;

    const std::vector<Move> moves = MoveGenerator::generateLegal(board);
    if (moves.empty()) {
        result.score = board.inCheck() ? -MateScore : 0;
    } else {
        int alpha = -Infinity;
        const int beta = Infinity;
        result.score = -Infinity;
        for (const Move& move : moves) {
            board.makeMove(move);
            const int score = -negamax(board, depth - 1, -beta, -alpha, 1);
            board.unmakeMove();
            // Strict '>' keeps the FIRST best move, which makes the search
            // deterministic and identical between Minimax and AlphaBeta.
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
    result.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    return result;
}

// Returns the score of `board` from the side to move's point of view.
// Alpha-beta invariant: we only care about scores in the window (alpha, beta).
//   alpha = the best score the side to move is already guaranteed elsewhere
//   beta  = the best score the OPPONENT is already guaranteed elsewhere
// If we find a move scoring >= beta, the opponent will never allow this
// position, so the remaining moves need not be searched (a "beta cutoff").
int Search::negamax(Board& board, int depth, int alpha, int beta, int ply) {
    ++nodes_;
    if (depth == 0) {
        return evaluator_.evaluate(board);
    }

    const std::vector<Move> moves = MoveGenerator::generateLegal(board);
    if (moves.empty()) {
        // Checkmate (bad for the side to move, worse the sooner) or stalemate.
        return board.inCheck() ? -MateScore + ply : 0;
    }
    if (board.halfmoveClock() >= 100) {
        return 0;  // fifty-move rule (checked after mate, which takes precedence)
    }

    int best = -Infinity;
    for (const Move& move : moves) {
        board.makeMove(move);
        const int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        board.unmakeMove();

        best = std::max(best, score);
        if (algorithm_ == Algorithm::AlphaBeta) {
            alpha = std::max(alpha, best);
            if (alpha >= beta) {
                break;  // beta cutoff
            }
        }
    }
    return best;
}

}  // namespace chess
