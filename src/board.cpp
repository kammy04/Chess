// =============================================================================
// board.cpp: definitions for include/chess/board.hpp
// =============================================================================
// New C++ ideas in this file (each explained where it first appears):
//   anonymous namespace, switch, exceptions (throw), loops (for / while),
//   string streams, lambdas, structured bindings, assert, bit operators.

#include "chess/board.hpp"

#include <cassert>    // assert(): a self-check that stops the program if it fails
#include <charconv>   // std::from_chars: fast text -> number conversion
#include <sstream>    // std::istringstream / std::ostringstream: streams over text
#include <stdexcept>  // std::invalid_argument: a standard exception type

#include "offsets.hpp"  // private header in src/ (knight/king/rook/bishop directions)

namespace chess {

// An ANONYMOUS namespace (one with no name). Anything inside is visible ONLY
// in this .cpp file. Use it for helpers nobody else should call. Other files
// can't see them, so they can't depend on them, and names here can't clash
// with names elsewhere.
namespace {

// Converts a Color to 0 or 1 for use as a list position. (constexpr, noexcept:
// see types.hpp.)
constexpr std::size_t colorIndex(Color c) noexcept { return static_cast<std::size_t>(c); }

// Several constants in one statement, separated by commas. These are the
// square numbers of the corners and king starting squares.
constexpr Square A1 = 0, E1 = 4, H1 = 7, A8 = 56, E8 = 60, H8 = 63;

// Which castling rights survive a move that touches square `sq`.
// Moving a king or rook, or capturing a rook on its starting square, loses
// the matching right(s) forever.
//
// SWITCH: compares one value against several fixed options.
//   switch (x) {
//       case 1: ...    runs if x == 1
//       case 2: ...    runs if x == 2
//       default: ...   runs if nothing above matched
//   }
// Each `case` here ends with `return`, which leaves the function, so the
// switch is done. (Without return or `break;`, execution would "fall through"
// into the next case.)
//
// BIT OPERATORS (on the binary digits of a number; see LEARNING.md):
//   a | b   OR:  a bit is on if it's on in a OR in b   (combine flags)
//   a & b   AND: a bit is on if it's on in BOTH        (keep only shared flags)
//   ~a      NOT: flips every bit                       (everything EXCEPT a)
// So `All & ~WhiteQueenside` means "every right except White-queenside".
constexpr std::uint8_t castlingMaskFor(Square sq) noexcept {
    // `using namespace castling;` lets us write WhiteKingside instead of
    // castling::WhiteKingside, inside this function only.
    using namespace castling;
    switch (sq) {
        case A1: return static_cast<std::uint8_t>(All & ~WhiteQueenside);
        case E1: return static_cast<std::uint8_t>(All & ~(WhiteKingside | WhiteQueenside));
        case H1: return static_cast<std::uint8_t>(All & ~WhiteKingside);
        case A8: return static_cast<std::uint8_t>(All & ~BlackQueenside);
        case E8: return static_cast<std::uint8_t>(All & ~(BlackKingside | BlackQueenside));
        case H8: return static_cast<std::uint8_t>(All & ~BlackKingside);
        default: return All;  // any other square: nothing lost
    }
}

// In an en-passant capture the captured pawn is NOT on the destination
// square; it's one row "behind" it (from the capturer's point of view).
constexpr Square enPassantVictim(Square to, Color mover) noexcept {
    return mover == Color::White ? to - 8 : to + 8;  // -8 = one row down, +8 = one row up
}

// EXCEPTIONS: how C++ reports an error that the current function can't handle.
//   throw SomeError("message");
// immediately stops the current function AND every function that called it,
// one by one, until some caller has a matching `try { } catch (...) { }`
// block (see app/main.cpp). If nobody catches it, the program ends.
//
// [[noreturn]] tells the compiler "this function never returns normally"
// (it always throws), so the compiler won't complain that code after a call
// to fenError() might be missing a return value.
//
// std::invalid_argument is a standard exception type meaning "you gave me bad
// input". The "+" joins strings to build the message. std::string(fen) makes
// a real string from the string_view so it can be joined.
[[noreturn]] void fenError(const std::string& reason, std::string_view fen) {
    throw std::invalid_argument("Invalid FEN (" + reason + "): " + std::string(fen));
}

// Turns text such as "12" into the number 12. Empty text gives `fallback`.
// Bad text (letters, negative numbers) throws via fenError.
int parseCounter(const std::string& text, int fallback, std::string_view fen) {
    if (text.empty()) {
        return fallback;
    }
    int value = 0;

    // POINTERS (brief; see LEARNING.md for more): a pointer holds the memory
    // ADDRESS of something. `const char*` is "address of a character we
    // won't modify". text.data() gives the address of the first character;
    // adding text.size() gives the address just past the last one.
    // std::from_chars needs those two addresses to know where the text is.
    const char* end = text.data() + text.size();

    // std::from_chars returns a small struct with two fields. A STRUCTURED
    // BINDING `auto [a, b] = ...` unpacks a struct's fields into separate
    // named variables in one go:
    //   ptr = where it stopped reading,  ec = an error code (empty if fine)
    const auto [ptr, ec] = std::from_chars(text.data(), end, value);

    // std::errc{} is "no error". If there was an error, or it stopped before
    // the end (e.g. "12x"), or the number is negative, the input is bad.
    if (ec != std::errc{} || ptr != end || value < 0) {
        fenError("bad move counter '" + text + "'", fen);
    }
    return value;
}

}  // namespace  (end of the anonymous namespace)

// -----------------------------------------------------------------------------
// Constructor and FEN loading
// -----------------------------------------------------------------------------

// The constructor's definition: `Board::Board()` reads "the Board function
// that belongs to Board", i.e. the constructor. It just loads the start position.
Board::Board() { loadFen(StartFen); }

Board Board::fromFen(std::string_view fen) {
    Board board;          // create a local Board (start position for now)...
    board.loadFen(fen);   // ...then overwrite it with the requested position
    // Returning a local object: C++ moves or builds it directly in the
    // caller's variable, so no expensive copy is made.
    // If loadFen threw an exception, we never get here, and the half-loaded
    // `board` is simply thrown away. The caller never sees a broken Board.
    return board;
}

void Board::loadFen(std::string_view fen) {
    // A STRING STREAM lets you read from text the same way you'd read from
    // the keyboard. `>>` reads the next word (stopping at spaces) into a
    // variable. Here we split the FEN into its space-separated fields.
    std::istringstream in{std::string(fen)};
    std::string placement, side, rights, ep, halfmove, fullmove;  // six empty strings

    // `in >> a >> b` reads a, then b. The whole expression counts as false if
    // any read failed (e.g. there weren't enough words). `!(...)` = "if NOT ok".
    if (!(in >> placement >> side >> rights >> ep)) {
        fenError("expected at least 4 fields", fen);
    }
    in >> halfmove >> fullmove;  // optional; stay empty if missing

    // Start from a completely clear board.
    squares_.fill(NoPiece);      // .fill(x) sets every element of the array to x
    kings_ = {NoSquare, NoSquare};
    history_.clear();            // .clear() empties the vector

    // 1. Piece placement. FEN lists rank 8 first, left to right, ranks
    //    separated by '/'. A digit means that many empty squares.
    int rank = 7;
    int file = 0;

    // RANGE-BASED FOR LOOP: "for each character c in placement, run the body".
    //   for (const char c : placement) { ... }
    // `const char c` is a fresh variable holding the current character on
    // each pass round the loop.
    for (const char c : placement) {
        if (c == '/') {
            // Single-line if bodies may skip the braces { }.
            if (file != 8) fenError("rank does not have 8 files", fen);
            // `--rank` subtracts 1 from rank (and ++x adds 1). Here we
            // subtract first, then test the new value.
            if (--rank < 0) fenError("too many ranks", fen);
            file = 0;
        } else if (c >= '1' && c <= '8') {  // `else if`: tried only if the previous test was false
            file += c - '0';  // '3' - '0' = 3: skip three empty squares
            if (file > 8) fenError("rank has more than 8 files", fen);
        } else {  // `else`: runs if none of the tests above were true
            // `piece` is a std::optional<Piece> (see types.hpp).
            const auto piece = pieceFromChar(c);
            if (!piece) fenError(std::string("unknown piece '") + c + "'", fen);  // empty = not a piece letter
            if (file >= 8) fenError("rank has more than 8 files", fen);
            // `piece->type` reads the `type` field of the Piece INSIDE the
            // optional. (-> is "follow, then take the field".)
            if (piece->type == PieceType::Pawn && (rank == 0 || rank == 7)) {
                fenError("pawn on first or last rank", fen);
            }
            const Square sq = makeSquare(file, rank);
            if (piece->type == PieceType::King) {
                if (kings_[colorIndex(piece->color)] != NoSquare) fenError("more than one king", fen);
                kings_[colorIndex(piece->color)] = sq;
            }
            squares_[sq] = *piece;  // `*piece` takes the Piece out of the optional
            ++file;
        }
    }
    if (rank != 0 || file != 8) fenError("placement must describe exactly 8 ranks", fen);
    if (kings_[0] == NoSquare || kings_[1] == NoSquare) fenError("each side needs a king", fen);

    // 2. Whose turn: "w" or "b". std::string can be compared to text with ==.
    if (side == "w") {
        side_ = Color::White;
    } else if (side == "b") {
        side_ = Color::Black;
    } else {
        fenError("side to move must be 'w' or 'b'", fen);
    }

    // 3. Castling rights, e.g. "KQkq" or "-" for none.
    castling_ = castling::None;
    if (rights != "-") {
        for (const char c : rights) {
            // `x |= flag` means x = x | flag: switch that flag's bit ON.
            // `break;` leaves the switch (otherwise the next case would run too).
            switch (c) {
                case 'K': castling_ |= castling::WhiteKingside; break;
                case 'Q': castling_ |= castling::WhiteQueenside; break;
                case 'k': castling_ |= castling::BlackKingside; break;
                case 'q': castling_ |= castling::BlackQueenside; break;
                default: fenError(std::string("bad castling character '") + c + "'", fen);
            }
        }
    }

    // A LAMBDA: a small function written inline and stored in a variable.
    //   [this](parameters) { body }
    // The [ ] part is the CAPTURE LIST: which outside variables the lambda may
    // use. `this` lets it use this Board's members (squares_, castling_).
    // (`this` is the built-in name for "the object whose member function is
    // running right now".) Call it like a normal function: keepIf(...).
    //
    // This one switches a castling right OFF if the king or rook isn't on its
    // starting square, so the rest of the program can trust castling_.
    const auto keepIf = [this](std::uint8_t right, Square king, Square rook, Color c) {
        if (!(squares_[king].is(c, PieceType::King) && squares_[rook].is(c, PieceType::Rook))) {
            // `x &= ~flag`: switch that flag's bit OFF, leave the rest alone.
            castling_ &= static_cast<std::uint8_t>(~right);
        }
    };
    keepIf(castling::WhiteKingside, E1, H1, Color::White);
    keepIf(castling::WhiteQueenside, E1, A1, Color::White);
    keepIf(castling::BlackKingside, E8, H8, Color::Black);
    keepIf(castling::BlackQueenside, E8, A8, Color::Black);

    // 4. En-passant square ("-" if none). Must be on the correct row, be
    //    empty, and have an enemy pawn right behind it.
    enPassant_ = NoSquare;
    if (ep != "-") {
        const auto sq = parseSquare(ep);
        if (!sq) fenError("bad en-passant square", fen);
        const int expectedRank = side_ == Color::White ? 5 : 2;
        const Square victim = enPassantVictim(*sq, side_);
        // `||` stops at the first true part (SHORT-CIRCUIT evaluation), so if
        // the rank is wrong we never look at squares_[victim], which might
        // not even be a real square in that case.
        if (rankOf(*sq) != expectedRank || !squares_[*sq].isEmpty() ||
            !squares_[victim].is(opposite(side_), PieceType::Pawn)) {
            fenError("impossible en-passant square", fen);
        }
        enPassant_ = *sq;
    }

    // 5. The two move counters (optional; default 0 and 1).
    halfmoveClock_ = parseCounter(halfmove, 0, fen);
    fullmoveNumber_ = parseCounter(fullmove, 1, fen);
    if (fullmoveNumber_ < 1) fenError("fullmove number must be >= 1", fen);

    // The player who just moved can't have left their own king in check.
    if (isSquareAttacked(kingSquare(opposite(side_)), side_)) {
        fenError("side not to move is in check", fen);
    }
}

// -----------------------------------------------------------------------------
// Writing FEN
// -----------------------------------------------------------------------------

std::string Board::positionKey() const {
    std::string key;  // starts as empty text ""

    // CLASSIC FOR LOOP:  for (start; keep-going-while; after-each-pass) { body }
    //   int rank = 7     create a counter starting at 7
    //   rank >= 0        repeat while this is true
    //   --rank           subtract 1 after each pass
    // So rank goes 7, 6, 5, ... 0. (FEN starts with rank 8, which is rank index 7.)
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;  // how many empty squares in a row we've seen
        for (int file = 0; file < 8; ++file) {  // file goes 0, 1, ... 7
            const Piece p = squares_[makeSquare(file, rank)];
            if (p.isEmpty()) {
                ++empty;
                continue;  // `continue` skips the rest of the body and goes to the next pass
            }
            if (empty > 0) {
                key += static_cast<char>('0' + empty);  // e.g. 3 empty squares -> '3'
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
        // `castling_ & flag` is non-zero (which counts as true) if that flag's bit is on.
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
    // std::to_string turns a number into text: 12 -> "12".
    return positionKey() + ' ' + std::to_string(halfmoveClock_) + ' ' +
           std::to_string(fullmoveNumber_);
}

// -----------------------------------------------------------------------------
// Attack detection
// -----------------------------------------------------------------------------
// Idea: instead of asking every enemy piece "can you reach sq?", stand ON sq
// and look outward like each piece type would. If a knight-jump away there's
// an enemy knight, it attacks sq, and so on.
bool Board::isSquareAttacked(Square sq, Color by) const {
    const int file = fileOf(sq);
    const int rank = rankOf(sq);

    // A lambda (see loadFen above). `[&]` captures EVERYTHING it uses BY
    // REFERENCE: it can see file, rank, by and this Board's members.
    // It answers: "is there a `type` piece of colour `by` at (f, r)?"
    // && short-circuits, so squares_ is only read when (f, r) is on the board.
    const auto holds = [&](int f, int r, PieceType type) {
        return onBoard(f, r) && squares_[makeSquare(f, r)].is(by, type);
    };

    // Pawns. A white pawn captures diagonally UPWARD, so a white pawn that
    // attacks sq must stand one rank BELOW it (and vice versa for black).
    const int pawnRank = by == Color::White ? rank - 1 : rank + 1;
    if (holds(file - 1, pawnRank, PieceType::Pawn) || holds(file + 1, pawnRank, PieceType::Pawn)) {
        return true;
    }

    // Range-for over the 8 knight jumps. Each element is an Offset {df, dr};
    // the structured binding `const auto& [df, dr]` unpacks its two fields
    // into variables named df and dr. (`const auto&`: look at each element
    // directly without copying it, and don't change it.)
    for (const auto& [df, dr] : detail::KnightOffsets) {
        if (holds(file + df, rank + dr, PieceType::Knight)) return true;
    }
    for (const auto& [df, dr] : detail::KingOffsets) {
        if (holds(file + df, rank + dr, PieceType::King)) return true;
    }

    // Sliding pieces: walk outward along each direction until we bump into
    // something. `const auto& directions` as a lambda parameter means this
    // lambda accepts ANY list type: it works for both the rook list and the
    // bishop list.
    const auto rayHits = [&](const auto& directions, PieceType slider) {
        for (const auto& [df, dr] : directions) {
            int f = file + df;
            int r = rank + dr;
            // WHILE LOOP: repeat the body as long as the condition is true.
            while (onBoard(f, r)) {
                const Piece p = squares_[makeSquare(f, r)];
                if (!p.isEmpty()) {
                    // First piece hit: does it attack along this line?
                    // (Queens move like both rooks and bishops.)
                    if (p.color == by && (p.type == slider || p.type == PieceType::Queen)) return true;
                    break;  // `break` leaves the while loop: this line is blocked
                }
                f += df;  // step one further along the line
                r += dr;
            }
        }
        return false;
    };
    return rayHits(detail::RookDirections, PieceType::Rook) ||
           rayHits(detail::BishopDirections, PieceType::Bishop);
}

bool Board::inCheck() const { return isSquareAttacked(kingSquare(side_), opposite(side_)); }

// -----------------------------------------------------------------------------
// Making and unmaking moves
// -----------------------------------------------------------------------------

void Board::makeMove(const Move& move) {
    const Piece moving = squares_[move.from];

    // assert(condition) is a SELF-CHECK for programmer mistakes: in a Debug
    // build, if the condition is false, the program stops immediately and
    // reports this line. In a Release (fast) build, asserts are removed
    // entirely. `&& "text"` is a trick to attach a message (text always counts as true).
    assert(!moving.isEmpty() && moving.color == side_ && "makeMove: no piece of the side to move");

    // Save everything needed to undo this move. UndoInfo{a, b, c, ...} fills
    // its fields in order (move, captured, castling, enPassant, halfmoveClock).
    UndoInfo undo{move, squares_[move.to], castling_, enPassant_, halfmoveClock_};
    if (move.flag == MoveFlag::EnPassant) {
        const Square victim = enPassantVictim(move.to, side_);
        undo.captured = squares_[victim];  // `.` reaches a field of a struct
        squares_[victim] = NoPiece;
    }
    // .push_back(x) adds x to the END of the vector, growing it by one.
    history_.push_back(undo);

    // Fifty-move-rule counter: back to 0 after a pawn move or capture, else +1.
    const bool capture = !undo.captured.isEmpty();
    halfmoveClock_ = (moving.type == PieceType::Pawn || capture) ? 0 : halfmoveClock_ + 1;

    // Put the piece on its new square (a promoting pawn becomes the new piece),
    // and empty the old square.
    squares_[move.to] = move.isPromotion() ? Piece{move.promotion, side_} : moving;
    squares_[move.from] = NoPiece;

    if (moving.type == PieceType::King) {
        kings_[colorIndex(side_)] = move.to;
        // Castling: the rook also moves, jumping to the other side of the king.
        if (move.flag == MoveFlag::CastleKingside) {
            squares_[move.from + 1] = squares_[move.from + 3];  // h-file rook -> f-file
            squares_[move.from + 3] = NoPiece;
        } else if (move.flag == MoveFlag::CastleQueenside) {
            squares_[move.from - 1] = squares_[move.from - 4];  // a-file rook -> d-file
            squares_[move.from - 4] = NoPiece;
        }
    }

    // Lose any castling rights this move affects (see castlingMaskFor).
    castling_ &= static_cast<std::uint8_t>(castlingMaskFor(move.from) & castlingMaskFor(move.to));

    // After a two-square pawn push, the square it jumped over becomes the
    // en-passant square (halfway between from and to). Otherwise there isn't one.
    enPassant_ = move.flag == MoveFlag::DoublePawnPush ? (move.from + move.to) / 2 : NoSquare;

    if (side_ == Color::Black) ++fullmoveNumber_;
    side_ = opposite(side_);  // the other player's turn
}

void Board::unmakeMove() {
    assert(!history_.empty() && "unmakeMove: nothing to undo");

    // .back() is the LAST element of the vector (the most recent move).
    // We COPY it into a local variable first, because .pop_back() removes
    // and destroys that last element. Using it afterwards would be a bug.
    const UndoInfo undo = history_.back();
    history_.pop_back();

    // A const reference to the move inside `undo`: a short name, no copy.
    const Move& move = undo.move;

    side_ = opposite(side_);  // back to the player who made the move
    if (side_ == Color::Black) --fullmoveNumber_;

    // A promoted piece turns back into a pawn when taken back.
    const Piece moved = move.isPromotion() ? Piece{PieceType::Pawn, side_} : squares_[move.to];
    squares_[move.from] = moved;

    if (move.flag == MoveFlag::EnPassant) {
        squares_[move.to] = NoPiece;
        squares_[enPassantVictim(move.to, side_)] = undo.captured;
    } else {
        squares_[move.to] = undo.captured;  // NoPiece if nothing was captured
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

// -----------------------------------------------------------------------------
// Drawing the board as text
// -----------------------------------------------------------------------------
std::string Board::render(bool unicode) const {
    // Pick the drawing characters. Each line is text ending in "\n" (newline).
    const std::string top = unicode ? "  ┌───┬───┬───┬───┬───┬───┬───┬───┐\n"
                                    : "  +---+---+---+---+---+---+---+---+\n";
    const std::string mid = unicode ? "  ├───┼───┼───┼───┼───┼───┼───┼───┤\n"
                                    : "  +---+---+---+---+---+---+---+---+\n";
    const std::string bottom = unicode ? "  └───┴───┴───┴───┴───┴───┴───┴───┘\n"
                                       : "  +---+---+---+---+---+---+---+---+\n";
    const std::string bar = unicode ? "│" : "|";
    const std::string files = "    a   b   c   d   e   f   g   h\n";

    // An OUTPUT string stream: you "print" into it with << just like std::cout,
    // and at the end .str() gives you everything printed as one std::string.
    std::ostringstream out;
    out << files << top;
    for (int rank = 7; rank >= 0; --rank) {  // top row (rank 8) first
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
