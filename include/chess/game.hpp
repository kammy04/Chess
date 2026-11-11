#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "chess/board.hpp"
#include "chess/move.hpp"

namespace chess {

enum class GameResult {
    Ongoing,
    WhiteWinsByCheckmate,
    BlackWinsByCheckmate,
    Stalemate,
    FiftyMoveRule,
    ThreefoldRepetition,
    InsufficientMaterial,
};

[[nodiscard]] std::string_view describe(GameResult result);

// A game in progress: the board plus everything that depends on the game's
// history rather than the current position alone (repetition, undo, reset).
//
// Board answers "what moves exist here?"; Game answers "is this move allowed
// now, and is the game over?". Keeping them separate means the search can use
// a bare Board without paying for history bookkeeping it doesn't need.
class Game {
public:
    Game();                           // standard starting position
    explicit Game(const Board& start); // any position, e.g. from FEN

    [[nodiscard]] const Board& board() const noexcept { return board_; }
    [[nodiscard]] const std::vector<Move>& moveHistory() const noexcept { return moves_; }
    [[nodiscard]] std::vector<Move> legalMoves() const;

    // Turns user text like "e2e4" or "e7e8q" into the matching legal move.
    // A promotion without a piece letter ("e7e8") defaults to a queen.
    [[nodiscard]] std::optional<Move> parseMove(std::string_view text) const;

    // Both return false (and change nothing) if the move is illegal or the
    // game is already over.
    bool play(const Move& move);
    bool play(std::string_view text);

    bool undo();  // false if there is nothing to undo
    void reset(); // back to the starting position of this game

    [[nodiscard]] GameResult result() const;
    [[nodiscard]] bool isOver() const { return result() != GameResult::Ongoing; }

private:
    [[nodiscard]] bool insufficientMaterial() const;

    Board start_;
    Board board_;
    std::vector<Move> moves_;
    std::vector<std::string> positionKeys_;  // one per position reached, including the start
};

}  // namespace chess
