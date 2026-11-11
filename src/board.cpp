#include "chess/board.hpp"

#include <cassert>
#include <charconv>
#include <sstream>
#include <stdexcept>

#include "offsets.hpp"

namespace chess {

// Anonymous namespace = internal linkage. These helpers are invisible outside
// this .cpp file, like `static` functions in C.
namespace {

constexpr std::size_t colorIndex(Color c) noexcept { return static_cast<std::size_t>(c); }

constexpr Square A1 = 0, E1 = 4, H1 = 7, A8 = 56, E8 = 60, H8 = 63;

// Which castling rights survive a move that touches `sq` (as origin or
// destination). Moving the king or a rook, or capturing a rook on its home
// square, removes the corresponding rights.
constexpr std::uint8_t castlingMaskFor(Square sq) noexcept {
    using namespace castling;
    switch (sq) {
        case A1: return static_cast<std::uint8_t>(All & ~WhiteQueenside);
        case E1: return static_cast<std::uint8_t>(All & ~(WhiteKingside | WhiteQueenside));
        case H1: return static_cast<std::uint8_t>(All & ~WhiteKingside);
        case A8: return static_cast<std::uint8_t>(All & ~BlackQueenside);
        case E8: return static_cast<std::uint8_t>(All & ~(BlackKingside | BlackQueenside));
        case H8: return static_cast<std::uint8_t>(All & ~BlackKingside);
        default: return All;
    }
}

// For an en-passant capture landing on `to`, the captured pawn sits one rank
// "behind" the destination from the mover's point of view.
constexpr Square enPassantVictim(Square to, Color mover) noexcept {
    return mover == Color::White ? to - 8 : to + 8;
}

[[noreturn]] void fenError(const std::string& reason, std::string_view fen) {
    throw std::invalid_argument("Invalid FEN (" + reason + "): " + std::string(fen));
}

int parseCounter(const std::string& text, int fallback, std::string_view fen) {
    if (text.empty()) {
        return fallback;
    }
    int value = 0;
    const char* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc{} || ptr != end || value < 0) {
        fenError("bad move counter '" + text + "'", fen);
    }
    return value;
}

}  // namespace

Board::Board() { loadFen(StartFen); }

Board Board::fromFen(std::string_view fen) {
    Board board;
    board.loadFen(fen);
    return board;  // returned by value: the compiler elides or moves, never deep-copies needlessly
}

void Board::loadFen(std::string_view fen) {
    std::istringstream in{std::string(fen)};
    std::string placement, side, rights, ep, halfmove, fullmove;
    if (!(in >> placement >> side >> rights >> ep)) {
        fenError("expected at least 4 fields", fen);
    }
    in >> halfmove >> fullmove;  // optional

    squares_.fill(NoPiece);
    kings_ = {NoSquare, NoSquare};
    history_.clear();

    // 1. Piece placement, rank 8 first.
    int rank = 7;
    int file = 0;
    for (const char c : placement) {
        if (c == '/') {
            if (file != 8) fenError("rank does not have 8 files", fen);
            if (--rank < 0) fenError("too many ranks", fen);
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) fenError("rank has more than 8 files", fen);
        } else {
            const auto piece = pieceFromChar(c);
            if (!piece) fenError(std::string("unknown piece '") + c + "'", fen);
            if (file >= 8) fenError("rank has more than 8 files", fen);
            if (piece->type == PieceType::Pawn && (rank == 0 || rank == 7)) {
                fenError("pawn on first or last rank", fen);
            }
            const Square sq = makeSquare(file, rank);
            if (piece->type == PieceType::King) {
                if (kings_[colorIndex(piece->color)] != NoSquare) fenError("more than one king", fen);
                kings_[colorIndex(piece->color)] = sq;
            }
            squares_[sq] = *piece;
            ++file;
        }
    }
    if (rank != 0 || file != 8) fenError("placement must describe exactly 8 ranks", fen);
    if (kings_[0] == NoSquare || kings_[1] == NoSquare) fenError("each side needs a king", fen);

    // 2. Side to move.
    if (side == "w") {
        side_ = Color::White;
    } else if (side == "b") {
        side_ = Color::Black;
    } else {
        fenError("side to move must be 'w' or 'b'", fen);
    }

    // 3. Castling rights. Rights whose king/rook are not on their home squares
    //    are dropped, so the rest of the engine can trust castling_.
    castling_ = castling::None;
    if (rights != "-") {
        for (const char c : rights) {
            switch (c) {
                case 'K': castling_ |= castling::WhiteKingside; break;
                case 'Q': castling_ |= castling::WhiteQueenside; break;
                case 'k': castling_ |= castling::BlackKingside; break;
                case 'q': castling_ |= castling::BlackQueenside; break;
                default: fenError(std::string("bad castling character '") + c + "'", fen);
            }
        }
    }
    const auto keepIf = [this](std::uint8_t right, Square king, Square rook, Color c) {
        if (!(squares_[king].is(c, PieceType::King) && squares_[rook].is(c, PieceType::Rook))) {
            castling_ &= static_cast<std::uint8_t>(~right);
        }
    };
    keepIf(castling::WhiteKingside, E1, H1, Color::White);
    keepIf(castling::WhiteQueenside, E1, A1, Color::White);
    keepIf(castling::BlackKingside, E8, H8, Color::Black);
    keepIf(castling::BlackQueenside, E8, A8, Color::Black);

    // 4. En-passant target square.
    enPassant_ = NoSquare;
    if (ep != "-") {
        const auto sq = parseSquare(ep);
        if (!sq) fenError("bad en-passant square", fen);
        const int expectedRank = side_ == Color::White ? 5 : 2;
        const Square victim = enPassantVictim(*sq, side_);
        if (rankOf(*sq) != expectedRank || !squares_[*sq].isEmpty() ||
            !squares_[victim].is(opposite(side_), PieceType::Pawn)) {
            fenError("impossible en-passant square", fen);
        }
        enPassant_ = *sq;
    }

    // 5. Counters.
    halfmoveClock_ = parseCounter(halfmove, 0, fen);
    fullmoveNumber_ = parseCounter(fullmove, 1, fen);
    if (fullmoveNumber_ < 1) fenError("fullmove number must be >= 1", fen);

    // The player who just moved cannot have left their own king in check.
    if (isSquareAttacked(kingSquare(opposite(side_)), side_)) {
        fenError("side not to move is in check", fen);
    }
}

std::string Board::positionKey() const {
    std::string key;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            const Piece p = squares_[makeSquare(file, rank)];
            if (p.isEmpty()) {
                ++empty;
                continue;
            }
            if (empty > 0) {
                key += static_cast<char>('0' + empty);
                empty = 0;
            }
            key += pieceToChar(p);
        }
        if (empty > 0) key += static_cast<char>('0' + empty);
        if (rank > 0) key += '/';
    }

    key += side_ == Color::White ? " w " : " b ";

    if (castling_ == castling::None) {
        key += '-';
    } else {
        if (castling_ & castling::WhiteKingside) key += 'K';
        if (castling_ & castling::WhiteQueenside) key += 'Q';
        if (castling_ & castling::BlackKingside) key += 'k';
        if (castling_ & castling::BlackQueenside) key += 'q';
    }
    key += ' ';
    key += enPassant_ == NoSquare ? "-" : squareToString(enPassant_);
    return key;
}

std::string Board::toFen() const {
    return positionKey() + ' ' + std::to_string(halfmoveClock_) + ' ' +
           std::to_string(fullmoveNumber_);
}

bool Board::isSquareAttacked(Square sq, Color by) const {
    const int file = fileOf(sq);
    const int rank = rankOf(sq);

    // Lambda capturing by reference: a small local function that can see
    // `by` and `this`. Kotlin: a local `fun` or a lambda.
    const auto holds = [&](int f, int r, PieceType type) {
        return onBoard(f, r) && squares_[makeSquare(f, r)].is(by, type);
    };

    // A white pawn attacks diagonally upward, so it sits one rank BELOW.
    const int pawnRank = by == Color::White ? rank - 1 : rank + 1;
    if (holds(file - 1, pawnRank, PieceType::Pawn) || holds(file + 1, pawnRank, PieceType::Pawn)) {
        return true;
    }
    for (const auto& [df, dr] : detail::KnightOffsets) {
        if (holds(file + df, rank + dr, PieceType::Knight)) return true;
    }
    for (const auto& [df, dr] : detail::KingOffsets) {
        if (holds(file + df, rank + dr, PieceType::King)) return true;
    }

    // Walk outward along each ray until we hit a piece.
    const auto rayHits = [&](const auto& directions, PieceType slider) {
        for (const auto& [df, dr] : directions) {
            int f = file + df;
            int r = rank + dr;
            while (onBoard(f, r)) {
                const Piece p = squares_[makeSquare(f, r)];
                if (!p.isEmpty()) {
                    if (p.color == by && (p.type == slider || p.type == PieceType::Queen)) return true;
                    break;  // blocked
                }
                f += df;
                r += dr;
            }
        }
        return false;
    };
    return rayHits(detail::RookDirections, PieceType::Rook) ||
           rayHits(detail::BishopDirections, PieceType::Bishop);
}

bool Board::inCheck() const { return isSquareAttacked(kingSquare(side_), opposite(side_)); }

void Board::makeMove(const Move& move) {
    const Piece moving = squares_[move.from];
    assert(!moving.isEmpty() && moving.color == side_ && "makeMove: no piece of the side to move");

    UndoInfo undo{move, squares_[move.to], castling_, enPassant_, halfmoveClock_};
    if (move.flag == MoveFlag::EnPassant) {
        const Square victim = enPassantVictim(move.to, side_);
        undo.captured = squares_[victim];
        squares_[victim] = NoPiece;
    }
    history_.push_back(undo);

    // Fifty-move rule counter: reset on any pawn move or capture.
    const bool capture = !undo.captured.isEmpty();
    halfmoveClock_ = (moving.type == PieceType::Pawn || capture) ? 0 : halfmoveClock_ + 1;

    squares_[move.to] = move.isPromotion() ? Piece{move.promotion, side_} : moving;
    squares_[move.from] = NoPiece;

    if (moving.type == PieceType::King) {
        kings_[colorIndex(side_)] = move.to;
        if (move.flag == MoveFlag::CastleKingside) {
            squares_[move.from + 1] = squares_[move.from + 3];  // h-rook -> f-file
            squares_[move.from + 3] = NoPiece;
        } else if (move.flag == MoveFlag::CastleQueenside) {
            squares_[move.from - 1] = squares_[move.from - 4];  // a-rook -> d-file
            squares_[move.from - 4] = NoPiece;
        }
    }

    castling_ &= static_cast<std::uint8_t>(castlingMaskFor(move.from) & castlingMaskFor(move.to));
    enPassant_ = move.flag == MoveFlag::DoublePawnPush ? (move.from + move.to) / 2 : NoSquare;

    if (side_ == Color::Black) ++fullmoveNumber_;
    side_ = opposite(side_);
}

void Board::unmakeMove() {
    assert(!history_.empty() && "unmakeMove: nothing to undo");
    const UndoInfo undo = history_.back();  // copy before pop_back destroys it
    history_.pop_back();
    const Move& move = undo.move;

    side_ = opposite(side_);  // back to the side that made the move
    if (side_ == Color::Black) --fullmoveNumber_;

    const Piece moved = move.isPromotion() ? Piece{PieceType::Pawn, side_} : squares_[move.to];
    squares_[move.from] = moved;

    if (move.flag == MoveFlag::EnPassant) {
        squares_[move.to] = NoPiece;
        squares_[enPassantVictim(move.to, side_)] = undo.captured;
    } else {
        squares_[move.to] = undo.captured;
    }

    if (moved.type == PieceType::King) {
        kings_[colorIndex(side_)] = move.from;
        if (move.flag == MoveFlag::CastleKingside) {
            squares_[move.from + 3] = squares_[move.from + 1];
            squares_[move.from + 1] = NoPiece;
        } else if (move.flag == MoveFlag::CastleQueenside) {
            squares_[move.from - 4] = squares_[move.from - 1];
            squares_[move.from - 1] = NoPiece;
        }
    }

    castling_ = undo.castling;
    enPassant_ = undo.enPassant;
    halfmoveClock_ = undo.halfmoveClock;
}

std::string Board::render(bool unicode) const {
    const std::string top = unicode ? "  ┌───┬───┬───┬───┬───┬───┬───┬───┐\n"
                                    : "  +---+---+---+---+---+---+---+---+\n";
    const std::string mid = unicode ? "  ├───┼───┼───┼───┼───┼───┼───┼───┤\n"
                                    : "  +---+---+---+---+---+---+---+---+\n";
    const std::string bottom = unicode ? "  └───┴───┴───┴───┴───┴───┴───┴───┘\n"
                                       : "  +---+---+---+---+---+---+---+---+\n";
    const std::string bar = unicode ? "│" : "|";
    const std::string files = "    a   b   c   d   e   f   g   h\n";

    std::ostringstream out;
    out << files << top;
    for (int rank = 7; rank >= 0; --rank) {
        out << (rank + 1) << ' ' << bar;
        for (int file = 0; file < 8; ++file) {
            out << ' ' << pieceSymbol(squares_[makeSquare(file, rank)], unicode) << ' ' << bar;
        }
        out << ' ' << (rank + 1) << '\n' << (rank > 0 ? mid : bottom);
    }
    out << files;
    return out.str();
}

}  // namespace chess
