// =============================================================================
// move.hpp: what a chess move looks like as data
// =============================================================================
// Read after types.hpp. New C++ ideas here: an enum with comments per value,
// a struct used as a simple data bundle, and teaching std::cout to print it.

#pragma once

#include <cstdint>
#include <ostream>  // std::ostream: the general "something you can print to" type
#include <string>

#include "chess/types.hpp"  // our own header: gives us Square, PieceType, etc.

namespace chess {

// The special kinds of move. Most moves are Normal, including ordinary
// captures: the board can see what is on the destination square, so we don't
// need to store "this is a capture" separately.
enum class MoveFlag : std::uint8_t {
    Normal,
    DoublePawnPush,   // pawn moves two squares from its starting row
    EnPassant,        // a special pawn capture (see LEARNING.md, chess glossary)
    CastleKingside,   // king moves two squares toward the h-file; rook jumps over
    CastleQueenside,  // same, toward the a-file
};  // a trailing comma after the last value is allowed and harmless

// A Move is a plain bundle of four values. Because every field has a default,
// you can create one in several ways:
//   Move{}                          all defaults: the "null move" (no move at all)
//   Move{from, to}                  from and to set, the rest default
//   Move{from, to, PieceType::Queen, MoveFlag::Normal}   everything given
// Values in { } fill the fields IN THE ORDER THEY ARE WRITTEN BELOW.
// A struct like this, with public fields and no hand-written constructor, is
// called an AGGREGATE.
struct Move {
    Square from = NoSquare;                 // where the piece starts
    Square to = NoSquare;                   // where it lands
    PieceType promotion = PieceType::None;  // what a pawn turns into on the last row, if anything
    MoveFlag flag = MoveFlag::Normal;

    // Small "question" member functions (see Piece::isEmpty in types.hpp for
    // what each keyword means).
    [[nodiscard]] constexpr bool isPromotion() const noexcept {
        return promotion != PieceType::None;
    }
    [[nodiscard]] constexpr bool isCastle() const noexcept {
        return flag == MoveFlag::CastleKingside || flag == MoveFlag::CastleQueenside;
    }
    [[nodiscard]] constexpr bool isNull() const noexcept { return from == NoSquare; }

    // Text form: "e2e4", or "e7e8q" for a promotion. The null move is "0000".
    // (This is the notation chess programs use to talk to each other, called UCI.)
    // Declared here; the body is in src/move.cpp.
    [[nodiscard]] std::string toUci() const;

    // Two moves are equal if all four fields are equal (see Piece in types.hpp).
    bool operator==(const Move&) const = default;
};

// Teaches C++ how to PRINT a Move, so you can write:
//     std::cout << move;        // prints e.g. e2e4
// std::cout is the standard OUTPUT stream (the terminal). `<<` sends a value
// into it. To support `<<` for our own type we write a function named
// `operator<<`. It receives the stream by reference (std::ostream&), writes
// to it, and returns the same stream, so several << can be chained:
//     std::cout << "Best: " << move << '\n';   // '\n' is a NEWLINE character
// It's also how the test framework prints a Move when a test fails.
std::ostream& operator<<(std::ostream& os, const Move& move);

}  // namespace chess
