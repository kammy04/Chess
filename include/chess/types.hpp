#pragma once

// Core vocabulary types shared by every part of the engine.
//
// C comparison: the legacy C version used bare ints (0 = empty, 1 = king, ...)
// with no colour at all. Here the type system carries that meaning, so a
// "white knight" is a value the compiler understands, not a magic number.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace chess {

// `enum class` is scoped and strongly typed: Color::White does not silently
// convert to int, and you cannot compare a Color with a PieceType by accident.
// Kotlin equivalent: `enum class Color { WHITE, BLACK }`.
enum class Color : std::uint8_t { White, Black };

constexpr Color opposite(Color c) noexcept {
    return c == Color::White ? Color::Black : Color::White;
}

// The numeric order matters: it indexes into lookup tables (values, symbols).
enum class PieceType : std::uint8_t { None, Pawn, Knight, Bishop, Rook, Queen, King };

// A small value type: copying it is as cheap as copying an int, so we pass it
// by value everywhere.
struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::White;

    [[nodiscard]] constexpr bool isEmpty() const noexcept { return type == PieceType::None; }
    [[nodiscard]] constexpr bool is(Color c, PieceType t) const noexcept {
        return type == t && color == c;
    }

    // C++20: ask the compiler to write member-wise equality for us.
    bool operator==(const Piece&) const = default;
};

inline constexpr Piece NoPiece{};

// Squares are numbered 0..63: a1 = 0, b1 = 1, ..., h1 = 7, a2 = 8, ..., h8 = 63.
// This "little-endian rank-file" layout is the one most engines (and later,
// bitboards) use, so learning it now pays off in Phase 3.
using Square = int;
inline constexpr Square NoSquare = -1;

constexpr int fileOf(Square sq) noexcept { return sq % 8; }  // 0 = a-file
constexpr int rankOf(Square sq) noexcept { return sq / 8; }  // 0 = rank 1
constexpr Square makeSquare(int file, int rank) noexcept { return rank * 8 + file; }
constexpr bool onBoard(int file, int rank) noexcept {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}
constexpr bool isValidSquare(Square sq) noexcept { return sq >= 0 && sq < 64; }

// "e4" <-> 28. parseSquare returns std::nullopt (Kotlin: null) for bad input
// instead of reading garbage the way the C version's `input[0] - 48` did.
[[nodiscard]] std::string squareToString(Square sq);
[[nodiscard]] std::optional<Square> parseSquare(std::string_view text);

// FEN letters: uppercase = white, lowercase = black ('P', 'n', ...). '.' = empty.
[[nodiscard]] char pieceToChar(Piece piece);
[[nodiscard]] std::optional<Piece> pieceFromChar(char c);

// Glyph used by Board::render, e.g. "♘" or "N".
[[nodiscard]] std::string_view pieceSymbol(Piece piece, bool unicode);

}  // namespace chess
