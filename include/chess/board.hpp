#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "chess/move.hpp"
#include "chess/types.hpp"

namespace chess {

// Castling rights as a 4-bit mask. A namespace of constants (instead of an
// enum class) keeps bitwise operators (&, |, ~) usable without casts.
namespace castling {
inline constexpr std::uint8_t WhiteKingside = 1U << 0U;
inline constexpr std::uint8_t WhiteQueenside = 1U << 1U;
inline constexpr std::uint8_t BlackKingside = 1U << 2U;
inline constexpr std::uint8_t BlackQueenside = 1U << 3U;
inline constexpr std::uint8_t None = 0;
inline constexpr std::uint8_t All = 0b1111;
}  // namespace castling

// The full game state needed to generate moves: piece placement, side to
// move, castling rights, en-passant square, and the two move counters.
//
// Board is a *value type*: copying a Board gives an independent board, and
// it cleans up after itself automatically (RAII). There is no malloc/free and
// no destructor to write: std::array and std::vector already manage their
// own memory, so the compiler-generated copy/move/destructor are correct.
// This is the "rule of zero".
class Board {
public:
    static constexpr std::string_view StartFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    // The standard starting position.
    Board();

    // Parses Forsyth-Edwards Notation. Throws std::invalid_argument with a
    // human-readable reason if the FEN is malformed or the position illegal
    // (e.g. two white kings, or the side NOT to move is in check).
    [[nodiscard]] static Board fromFen(std::string_view fen);

    [[nodiscard]] std::string toFen() const;

    // FEN without the move counters: two positions with equal keys are the
    // "same position" for the threefold-repetition rule.
    [[nodiscard]] std::string positionKey() const;

    // --- Queries. `const` member functions promise not to modify the board,
    // so they can be called through a `const Board&`.
    [[nodiscard]] Piece pieceAt(Square sq) const noexcept { return squares_[sq]; }
    [[nodiscard]] Color sideToMove() const noexcept { return side_; }
    [[nodiscard]] std::uint8_t castlingRights() const noexcept { return castling_; }
    [[nodiscard]] Square enPassantSquare() const noexcept { return enPassant_; }
    [[nodiscard]] int halfmoveClock() const noexcept { return halfmoveClock_; }
    [[nodiscard]] int fullmoveNumber() const noexcept { return fullmoveNumber_; }
    [[nodiscard]] Square kingSquare(Color c) const noexcept {
        return kings_[static_cast<std::size_t>(c)];
    }

    // True if any piece of colour `by` attacks `sq`.
    [[nodiscard]] bool isSquareAttacked(Square sq, Color by) const;

    // True if the side to move is in check.
    [[nodiscard]] bool inCheck() const;

    // --- Mutation. makeMove trusts that `move` is at least pseudo-legal for
    // this position (MoveGenerator produces only such moves). The state needed
    // to reverse it is pushed onto an internal stack, so unmakeMove() can undo
    // moves in LIFO order -- this is how the search explores the game tree
    // without copying the board at every node.
    void makeMove(const Move& move);
    void unmakeMove();
    [[nodiscard]] bool canUnmake() const noexcept { return !history_.empty(); }

    // Pretty-printed board, rank 8 at the top. unicode=false gives ASCII
    // letters for terminals without chess glyphs.
    [[nodiscard]] std::string render(bool unicode = true) const;

private:
    // Everything makeMove destroys that unmakeMove needs back.
    struct UndoInfo {
        Move move;
        Piece captured;
        std::uint8_t castling;
        Square enPassant;
        int halfmoveClock;
    };

    void loadFen(std::string_view fen);

    std::array<Piece, 64> squares_{};
    std::array<Square, 2> kings_{NoSquare, NoSquare};  // indexed by Color
    Color side_ = Color::White;
    std::uint8_t castling_ = castling::None;
    Square enPassant_ = NoSquare;
    int halfmoveClock_ = 0;
    int fullmoveNumber_ = 1;
    std::vector<UndoInfo> history_;
};

}  // namespace chess
