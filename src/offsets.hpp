// =============================================================================
// offsets.hpp: the directions each piece can move in
// =============================================================================
// This is a PRIVATE header. It lives in src/, not include/, so only the
// engine's own .cpp files can include it. Code that USES the engine never
// sees it. Hiding details like this is called ENCAPSULATION.

#pragma once

#include <array>

// A namespace inside a namespace: these names are chess::detail::Offset etc.
// "detail" is a common convention meaning "internal stuff, not for outsiders".
namespace chess::detail {

// One step on the board: how many files (columns) and ranks (rows) to move.
// For example {1, 2} is "one file right, two ranks up", a knight jump.
// Storing file/rank steps (rather than "add 17 to the square number") means a
// piece on the right edge can never wrap around to the left edge:
// onBoard() simply rejects the off-board result.
struct Offset {
    int df;  // "delta file": change in file
    int dr;  // "delta rank": change in rank
};

// A fixed list of 8 Offsets. The doubled braces {{ ... }} are needed because
// std::array is a struct with one array inside: the outer braces are for the
// std::array, the inner ones for its contents, and each {a, b} is one Offset.
inline constexpr std::array<Offset, 8> KnightOffsets{
    {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};

// The king steps one square in any of 8 directions.
inline constexpr std::array<Offset, 8> KingOffsets{
    {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}};

// Rooks slide in straight lines; bishops slide diagonally. Queens use both
// lists. "Slide" means repeat the step until something is in the way.
inline constexpr std::array<Offset, 4> RookDirections{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

inline constexpr std::array<Offset, 4> BishopDirections{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

}  // namespace chess::detail
