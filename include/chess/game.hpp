// =============================================================================
// game.hpp: a whole game: the board plus its history and the end-of-game rules
// =============================================================================
// Board answers "what's the position and what moves exist?".
// Game answers "is this move allowed right now, and is the game over?", which
// needs the HISTORY of the game (e.g. a draw when the same position appears
// three times). Keeping them apart means the search can use a plain, fast
// Board without paying for history it doesn't need.
//
// New C++ ideas: FUNCTION OVERLOADING (two functions with the same name),
// a delegating constructor, and a function that's declared but defined
// in a different file (describe).

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "chess/board.hpp"
#include "chess/move.hpp"

namespace chess {

// The possible states of a game.
enum class GameResult {
    Ongoing,
    WhiteWinsByCheckmate,
    BlackWinsByCheckmate,
    Stalemate,             // no legal moves but not in check: a draw
    FiftyMoveRule,         // 50 moves each with no capture or pawn move: a draw
    ThreefoldRepetition,   // same position 3 times: a draw
    InsufficientMaterial,  // nobody has enough pieces left to checkmate: a draw
};

// A FREE FUNCTION (not inside any class): turns a result into readable text.
[[nodiscard]] std::string_view describe(GameResult result);

class Game {
public:
    Game();                             // a new game from the standard start
    explicit Game(const Board& start);  // a new game from any position (e.g. loaded from FEN)

    // Getters returning CONST REFERENCES: the caller gets read-only access to
    // our actual Board / move list, with no copy made.
    [[nodiscard]] const Board& board() const noexcept { return board_; }
    [[nodiscard]] const std::vector<Move>& moveHistory() const noexcept { return moves_; }
    [[nodiscard]] std::vector<Move> legalMoves() const;

    // Turns typed text like "e2e4" or "e7e8q" into the matching legal move,
    // or returns nothing (an empty optional) if it isn't one. "e7e8" without
    // a letter means promote to a queen.
    [[nodiscard]] std::optional<Move> parseMove(std::string_view text) const;

    // FUNCTION OVERLOADING: two functions named `play` with different
    // parameter types. C++ picks the right one from what you pass:
    //   game.play(someMove);   // calls the first
    //   game.play("e2e4");     // calls the second
    // Both return true if the move was played, false (changing nothing) if
    // it's illegal or the game is already over.
    bool play(const Move& move);
    bool play(std::string_view text);

    bool undo();   // take back the last move; false if there's nothing to undo
    void reset();  // go back to this game's starting position

    [[nodiscard]] GameResult result() const;
    [[nodiscard]] bool isOver() const { return result() != GameResult::Ongoing; }

private:
    [[nodiscard]] bool insufficientMaterial() const;

    Board start_;                            // where this game began (for reset)
    Board board_;                            // the current position
    std::vector<Move> moves_;                // every move played, in order
    std::vector<std::string> positionKeys_;  // every position reached (for repetition)
};

}  // namespace chess
