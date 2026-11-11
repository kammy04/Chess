#pragma once

#include <cstdint>
#include <ostream>
#include <string>

#include "chess/types.hpp"

namespace chess {

// Special-move kinds. Captures are not a flag: the board knows what was on
// the destination square, so storing it here would duplicate state.
enum class MoveFlag : std::uint8_t {
    Normal,
    DoublePawnPush,  // sets the en-passant square
    EnPassant,       // captured pawn is NOT on the destination square
    CastleKingside,  // rook also moves h-file -> f-file
    CastleQueenside, // rook also moves a-file -> d-file
};

// A plain aggregate: no constructor needed, build it with braces:
//   Move m{from, to, PieceType::None, MoveFlag::Normal};
// Default member initialisers mean Move{} is a well-defined "null move".
struct Move {
    Square from = NoSquare;
    Square to = NoSquare;
    PieceType promotion = PieceType::None;
    MoveFlag flag = MoveFlag::Normal;

    [[nodiscard]] constexpr bool isPromotion() const noexcept {
        return promotion != PieceType::None;
    }
    [[nodiscard]] constexpr bool isCastle() const noexcept {
        return flag == MoveFlag::CastleKingside || flag == MoveFlag::CastleQueenside;
    }
    [[nodiscard]] constexpr bool isNull() const noexcept { return from == NoSquare; }

    // UCI / long algebraic notation: "e2e4", "e7e8q". The null move is "0000".
    [[nodiscard]] std::string toUci() const;

    bool operator==(const Move&) const = default;
};

// Lets GoogleTest (and std::cout) print moves readably.
std::ostream& operator<<(std::ostream& os, const Move& move);

}  // namespace chess
