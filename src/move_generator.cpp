// =============================================================================
// move_generator.cpp: definitions for include/chess/move_generator.hpp
// =============================================================================
// New C++ ideas: std::span (a view of a list), passing a vector by reference
// so a function can ADD to it, brace-initializing a Move inside push_back,
// recursion (a function calling itself).

#include "chess/move_generator.hpp"

#include <span>  // std::span

#include "offsets.hpp"

namespace chess {
namespace {  // private helpers (see the anonymous namespace in board.cpp)

// `using detail::Offset;` lets us write Offset instead of detail::Offset here.
using detail::Offset;

// The four pieces a pawn may become on the last rank.
constexpr std::array<PieceType, 4> PromotionPieces{PieceType::Queen, PieceType::Rook,
                                                   PieceType::Bishop, PieceType::Knight};

// Adds every pawn move from square `from` to the list `moves`.
// `std::vector<Move>& moves` is a NON-const reference: we receive the
// caller's real list and push new moves onto it. This avoids creating and
// joining many small lists.
void addPawnMoves(const Board& board, Square from, std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    const int forward = us == Color::White ? 1 : -1;     // White moves up the board, Black down
    const int startRank = us == Color::White ? 1 : 6;    // the row pawns start on
    const int promotionRank = us == Color::White ? 7 : 0;
    const int file = fileOf(from);
    const int nextRank = rankOf(from) + forward;  // always on the board: a pawn is never on its last rank

    // A lambda that adds one move, or FOUR if the pawn reaches the last rank
    // (one per promotion piece). `[&]` lets it use from, moves, etc.
    const auto add = [&](Square to, MoveFlag flag) {
        if (rankOf(to) == promotionRank) {
            for (const PieceType piece : PromotionPieces) {
                // push_back({...}): the braces build a Move right there, filling
                // its fields in order: from, to, promotion, flag.
                moves.push_back({from, to, piece, MoveFlag::Normal});
            }
        } else {
            moves.push_back({from, to, PieceType::None, flag});
        }
    };

    // One step forward, only onto an EMPTY square (pawns can't capture straight ahead).
    const Square oneStep = makeSquare(file, nextRank);
    if (board.pieceAt(oneStep).isEmpty()) {
        add(oneStep, MoveFlag::Normal);
        // Two steps, only from the starting row and only if both squares are empty.
        if (rankOf(from) == startRank) {
            const Square twoSteps = makeSquare(file, nextRank + forward);
            if (board.pieceAt(twoSteps).isEmpty()) {
                add(twoSteps, MoveFlag::DoublePawnPush);
            }
        }
    }

    // Diagonal captures, to the left (-1) and right (+1).
    // `{-1, 1}` is a quick inline list; the loop runs once with df = -1 and once with df = 1.
    for (const int df : {-1, 1}) {
        if (!onBoard(file + df, nextRank)) continue;  // off the edge: skip this side
        const Square to = makeSquare(file + df, nextRank);
        const Piece target = board.pieceAt(to);
        if (!target.isEmpty() && target.color != us) {
            add(to, MoveFlag::Normal);           // ordinary capture
        } else if (to == board.enPassantSquare()) {
            add(to, MoveFlag::EnPassant);        // en-passant capture
        }
    }
}

// Knights and kings: they "leap" one step in each listed direction.
//
// std::span<const Offset> is a VIEW of a list of Offsets: it points at a list
// that exists somewhere else (and knows its length) without copying it. Its
// advantage here: one function accepts both an std::array of 8 Offsets and
// one of 4, because span doesn't care about the size.
void addLeaperMoves(const Board& board, Square from, std::span<const Offset> offsets,
                    std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    for (const auto& [df, dr] : offsets) {  // structured binding: see board.cpp
        const int f = fileOf(from) + df;
        const int r = rankOf(from) + dr;
        if (!onBoard(f, r)) continue;
        const Square to = makeSquare(f, r);
        const Piece target = board.pieceAt(to);
        // Allowed onto an empty square or an enemy piece, never onto our own piece.
        if (target.isEmpty() || target.color != us) {
            moves.push_back({from, to, PieceType::None, MoveFlag::Normal});
        }
    }
}

// Bishops, rooks, queens: slide along each direction until blocked.
void addSliderMoves(const Board& board, Square from, std::span<const Offset> directions,
                    std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    for (const auto& [df, dr] : directions) {
        int f = fileOf(from) + df;
        int r = rankOf(from) + dr;
        while (onBoard(f, r)) {
            const Square to = makeSquare(f, r);
            const Piece target = board.pieceAt(to);
            if (target.isEmpty()) {
                moves.push_back({from, to, PieceType::None, MoveFlag::Normal});
            } else {
                if (target.color != us) {
                    moves.push_back({from, to, PieceType::None, MoveFlag::Normal});  // capture it
                }
                break;  // can't go past a piece, whoever's it is
            }
            f += df;
            r += dr;
        }
    }
}

// Castling. Allowed only if: the right still exists, the squares between king
// and rook are empty, and the king is not in check, does not pass through an
// attacked square, and does not land on one.
void addCastlingMoves(const Board& board, std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    const Color them = opposite(us);
    const int rank = us == Color::White ? 0 : 7;  // our back row
    const Square kingFrom = makeSquare(4, rank);   // the e-file

    // Three tiny lambdas to keep the rules below readable.
    // `[rank]` captures just `rank`, BY VALUE (its own copy).
    const auto sq = [rank](int file) { return makeSquare(file, rank); };
    const auto empty = [&](int file) { return board.pieceAt(sq(file)).isEmpty(); };
    const auto safe = [&](int file) { return !board.isSquareAttacked(sq(file), them); };

    // Board guarantees that a castling right implies king and rook are in place.
    // `(rights & flag) != 0` is true when that flag's bit is on.
    const std::uint8_t rights = board.castlingRights();
    const bool kingside = us == Color::White ? (rights & castling::WhiteKingside) != 0
                                             : (rights & castling::BlackKingside) != 0;
    const bool queenside = us == Color::White ? (rights & castling::WhiteQueenside) != 0
                                              : (rights & castling::BlackQueenside) != 0;

    // `return;` in a void function just stops it early.
    if ((!kingside && !queenside) || !safe(4)) return;  // no rights, or currently in check
    if (kingside && empty(5) && empty(6) && safe(5) && safe(6)) {
        moves.push_back({kingFrom, sq(6), PieceType::None, MoveFlag::CastleKingside});
    }
    // Queenside: b, c, d must be empty, but only c and d must be safe (the king
    // never crosses b).
    if (queenside && empty(1) && empty(2) && empty(3) && safe(3) && safe(2)) {
        moves.push_back({kingFrom, sq(2), PieceType::None, MoveFlag::CastleQueenside});
    }
}

}  // namespace

// `MoveGenerator::` in front of the name: this defines the static member
// function declared in the class. (The word `static` is NOT repeated here.)
std::vector<Move> MoveGenerator::generatePseudoLegal(const Board& board) {
    std::vector<Move> moves;  // an empty list

    // .reserve(64) asks the vector to set aside room for 64 moves up front.
    // A vector that outgrows its space must move everything to a bigger
    // block of memory; reserving avoids doing that repeatedly. Most chess
    // positions have 20 to 40 moves.
    moves.reserve(64);
    const Color us = board.sideToMove();

    for (Square sq = 0; sq < 64; ++sq) {  // visit every square
        const Piece piece = board.pieceAt(sq);
        if (piece.isEmpty() || piece.color != us) continue;  // not our piece

        switch (piece.type) {
            case PieceType::Pawn: addPawnMoves(board, sq, moves); break;
            // Passing detail::KnightOffsets (an std::array) where a std::span
            // is expected: C++ converts it automatically into a view of that array.
            case PieceType::Knight: addLeaperMoves(board, sq, detail::KnightOffsets, moves); break;
            case PieceType::Bishop: addSliderMoves(board, sq, detail::BishopDirections, moves); break;
            case PieceType::Rook: addSliderMoves(board, sq, detail::RookDirections, moves); break;
            case PieceType::Queen:
                addSliderMoves(board, sq, detail::RookDirections, moves);
                addSliderMoves(board, sq, detail::BishopDirections, moves);
                break;
            case PieceType::King:
                addLeaperMoves(board, sq, detail::KingOffsets, moves);
                addCastlingMoves(board, moves);
                break;
            case PieceType::None: break;
        }
    }
    // Returning a local vector doesn't copy all its elements: C++ hands the
    // finished list straight to the caller.
    return moves;
}

std::vector<Move> MoveGenerator::generateLegal(Board& board) {
    const Color us = board.sideToMove();
    std::vector<Move> legal;
    // Loop over the pseudo-legal list; keep the moves that don't leave our king in check.
    for (const Move& move : generatePseudoLegal(board)) {
        board.makeMove(move);
        // After makeMove it is the OPPONENT's turn, so board.sideToMove() is
        // now them. Question: is OUR king attacked by THEM?
        if (!board.isSquareAttacked(board.kingSquare(us), board.sideToMove())) {
            legal.push_back(move);
        }
        board.unmakeMove();  // always take it back
    }
    return legal;
}

// RECURSION: perft calls itself with depth - 1, going one move deeper each
// time, until depth reaches 0 or 1. Each call has its own `moves` and `nodes`.
std::uint64_t MoveGenerator::perft(Board& board, int depth) {
    if (depth <= 0) return 1;
    const std::vector<Move> moves = generateLegal(board);
    // Shortcut ("bulk counting"): at depth 1 the answer is just how many moves
    // there are, so no need to make and unmake each one.
    // .size() returns the number of elements.
    if (depth == 1) return moves.size();

    std::uint64_t nodes = 0;
    for (const Move& move : moves) {
        board.makeMove(move);
        nodes += perft(board, depth - 1);  // count everything below this move
        board.unmakeMove();
    }
    return nodes;
}

std::vector<std::pair<Move, std::uint64_t>> MoveGenerator::perftDivide(Board& board, int depth) {
    std::vector<std::pair<Move, std::uint64_t>> result;
    for (const Move& move : generateLegal(board)) {
        board.makeMove(move);
        // .emplace_back(a, b) is like push_back, but builds the new element
        // (here a pair) directly inside the vector from a and b.
        result.emplace_back(move, perft(board, depth - 1));
        board.unmakeMove();
    }
    return result;
}

}  // namespace chess
