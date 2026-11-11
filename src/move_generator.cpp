#include "chess/move_generator.hpp"

#include <span>

#include "offsets.hpp"

namespace chess {
namespace {

using detail::Offset;

constexpr std::array<PieceType, 4> PromotionPieces{PieceType::Queen, PieceType::Rook,
                                                   PieceType::Bishop, PieceType::Knight};

void addPawnMoves(const Board& board, Square from, std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    const int forward = us == Color::White ? 1 : -1;
    const int startRank = us == Color::White ? 1 : 6;
    const int promotionRank = us == Color::White ? 7 : 0;
    const int file = fileOf(from);
    const int nextRank = rankOf(from) + forward;  // always on board: pawns never sit on the last rank

    // Pushing to the last rank creates four moves, one per promotion piece.
    const auto add = [&](Square to, MoveFlag flag) {
        if (rankOf(to) == promotionRank) {
            for (const PieceType piece : PromotionPieces) {
                moves.push_back({from, to, piece, MoveFlag::Normal});
            }
        } else {
            moves.push_back({from, to, PieceType::None, flag});
        }
    };

    const Square oneStep = makeSquare(file, nextRank);
    if (board.pieceAt(oneStep).isEmpty()) {
        add(oneStep, MoveFlag::Normal);
        if (rankOf(from) == startRank) {
            const Square twoSteps = makeSquare(file, nextRank + forward);
            if (board.pieceAt(twoSteps).isEmpty()) {
                add(twoSteps, MoveFlag::DoublePawnPush);
            }
        }
    }

    for (const int df : {-1, 1}) {
        if (!onBoard(file + df, nextRank)) continue;
        const Square to = makeSquare(file + df, nextRank);
        const Piece target = board.pieceAt(to);
        if (!target.isEmpty() && target.color != us) {
            add(to, MoveFlag::Normal);
        } else if (to == board.enPassantSquare()) {
            add(to, MoveFlag::EnPassant);
        }
    }
}

// std::span (C++20) is a non-owning view of contiguous elements: pointer +
// length in one object. It lets one function accept std::array<Offset, 8>
// AND std::array<Offset, 4>. The C equivalent is passing (Offset*, size_t).
void addLeaperMoves(const Board& board, Square from, std::span<const Offset> offsets,
                    std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    for (const auto& [df, dr] : offsets) {
        const int f = fileOf(from) + df;
        const int r = rankOf(from) + dr;
        if (!onBoard(f, r)) continue;
        const Square to = makeSquare(f, r);
        const Piece target = board.pieceAt(to);
        if (target.isEmpty() || target.color != us) {
            moves.push_back({from, to, PieceType::None, MoveFlag::Normal});
        }
    }
}

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
                    moves.push_back({from, to, PieceType::None, MoveFlag::Normal});
                }
                break;  // can't slide through a piece (the C version's bug)
            }
            f += df;
            r += dr;
        }
    }
}

void addCastlingMoves(const Board& board, std::vector<Move>& moves) {
    const Color us = board.sideToMove();
    const Color them = opposite(us);
    const int rank = us == Color::White ? 0 : 7;
    const Square kingFrom = makeSquare(4, rank);
    const auto sq = [rank](int file) { return makeSquare(file, rank); };
    const auto empty = [&](int file) { return board.pieceAt(sq(file)).isEmpty(); };
    const auto safe = [&](int file) { return !board.isSquareAttacked(sq(file), them); };

    // Board::loadFen and makeMove guarantee rights imply king + rook at home.
    const std::uint8_t rights = board.castlingRights();
    const bool kingside = us == Color::White ? (rights & castling::WhiteKingside) != 0
                                             : (rights & castling::BlackKingside) != 0;
    const bool queenside = us == Color::White ? (rights & castling::WhiteQueenside) != 0
                                              : (rights & castling::BlackQueenside) != 0;

    // You may not castle out of, through, or into check. The destination
    // square is re-checked by the legality filter, but checking it here is
    // cheap and keeps the rule readable in one place.
    if ((!kingside && !queenside) || !safe(4)) return;
    if (kingside && empty(5) && empty(6) && safe(5) && safe(6)) {
        moves.push_back({kingFrom, sq(6), PieceType::None, MoveFlag::CastleKingside});
    }
    if (queenside && empty(1) && empty(2) && empty(3) && safe(3) && safe(2)) {
        moves.push_back({kingFrom, sq(2), PieceType::None, MoveFlag::CastleQueenside});
    }
}

}  // namespace

std::vector<Move> MoveGenerator::generatePseudoLegal(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(64);  // typical positions have 30-40 moves: avoid repeated reallocation
    const Color us = board.sideToMove();

    for (Square sq = 0; sq < 64; ++sq) {
        const Piece piece = board.pieceAt(sq);
        if (piece.isEmpty() || piece.color != us) continue;

        switch (piece.type) {
            case PieceType::Pawn: addPawnMoves(board, sq, moves); break;
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
    return moves;  // NRVO: no copy is made
}

std::vector<Move> MoveGenerator::generateLegal(Board& board) {
    const Color us = board.sideToMove();
    std::vector<Move> legal;
    for (const Move& move : generatePseudoLegal(board)) {
        board.makeMove(move);
        // After makeMove it is the opponent's turn; is OUR king attacked by them?
        if (!board.isSquareAttacked(board.kingSquare(us), board.sideToMove())) {
            legal.push_back(move);
        }
        board.unmakeMove();
    }
    return legal;
}

std::uint64_t MoveGenerator::perft(Board& board, int depth) {
    if (depth <= 0) return 1;
    const std::vector<Move> moves = generateLegal(board);
    if (depth == 1) return moves.size();  // "bulk counting": skip the last make/unmake

    std::uint64_t nodes = 0;
    for (const Move& move : moves) {
        board.makeMove(move);
        nodes += perft(board, depth - 1);
        board.unmakeMove();
    }
    return nodes;
}

std::vector<std::pair<Move, std::uint64_t>> MoveGenerator::perftDivide(Board& board, int depth) {
    std::vector<std::pair<Move, std::uint64_t>> result;
    for (const Move& move : generateLegal(board)) {
        board.makeMove(move);
        result.emplace_back(move, perft(board, depth - 1));
        board.unmakeMove();
    }
    return result;
}

}  // namespace chess
