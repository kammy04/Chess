#pragma once

// Private header: lives in src/, not include/, so it is NOT part of the
// library's public API. Only the .cpp files that need it can see it.

#include <array>

namespace chess::detail {

// (file delta, rank delta). Using file/rank deltas instead of raw index
// deltas (+17, -15, ...) means a knight on the h-file can never "wrap
// around" to the a-file: onBoard() rejects it.
struct Offset {
    int df;
    int dr;
};

inline constexpr std::array<Offset, 8> KnightOffsets{
    {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};

inline constexpr std::array<Offset, 8> KingOffsets{
    {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}};

inline constexpr std::array<Offset, 4> RookDirections{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

inline constexpr std::array<Offset, 4> BishopDirections{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

}  // namespace chess::detail
