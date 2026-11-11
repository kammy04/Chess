// =============================================================================
// board.hpp: the chess position: where every piece is, and whose turn it is
// =============================================================================
// Read after types.hpp and move.hpp. New C++ ideas in this file:
//   * CLASS, with public and private parts
//   * CONSTRUCTOR (the function that sets up a new object)
//   * STATIC member function (belongs to the class, not to one object)
//   * REFERENCES (the & symbol), explained in detail below
//   * std::array and std::vector (lists), and how memory is cleaned up
//     automatically (RAII)

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>  // std::vector: a list that can grow and shrink

#include "chess/move.hpp"
#include "chess/types.hpp"

namespace chess {

// Castling rights stored as BIT FLAGS: four yes/no answers packed into the
// four lowest bits of one byte. See LEARNING.md "Bits and bitwise operators".
//   1U << 0U   the number 1 shifted left by 0 places = binary 0001 = 1
//   1U << 1U   shifted left by 1 place               = binary 0010 = 2
//   1U << 2U   = 0100 = 4,   1U << 3U = 1000 = 8
// The U suffix means "unsigned number". 0b1111 is binary for 15: all four bits on.
// A plain namespace of constants is used (not an enum class) because we
// want to combine them with bit operators like | and &, which enum class
// would forbid without extra conversions.
namespace castling {
inline constexpr std::uint8_t WhiteKingside = 1U << 0U;
inline constexpr std::uint8_t WhiteQueenside = 1U << 1U;
inline constexpr std::uint8_t BlackKingside = 1U << 2U;
inline constexpr std::uint8_t BlackQueenside = 1U << 3U;
inline constexpr std::uint8_t None = 0;
inline constexpr std::uint8_t All = 0b1111;
}  // namespace castling

// -----------------------------------------------------------------------------
// CLASS
// -----------------------------------------------------------------------------
// A class is like a struct (a bundle of data plus functions) but it HIDES
// its data. Code outside the class can only use the `public:` part. The
// `private:` part (at the bottom) can only be touched by the class's own
// member functions.
//
// Why hide it? A Board has RULES that must always hold, for example "kings_
// always says where the kings really are". If any code anywhere could edit
// squares_ directly, it could break that rule. With the data private, only
// the Board's own functions change it, so only they need to get it right.
// (The only real difference between `struct` and `class` in C++: members of
// a struct are public unless you say otherwise; members of a class are private.)
//
// An OBJECT is one actual Board made from this class, e.g.  Board board;
// You can have many Board objects, each with its own pieces.
class Board {
public:  // ---- everything from here to `private:` is usable by anyone --------

    // A constant belonging to the class, written Board::StartFen from outside.
    // `static` here means "one shared value for the class, not one per Board".
    // FEN is a one-line text description of a position (see LEARNING.md).
    static constexpr std::string_view StartFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    // A CONSTRUCTOR: a special function with the SAME NAME as the class and
    // NO return type. It runs automatically whenever a Board is created and
    // sets it up. This one sets up the standard starting position:
    //     Board board;     // constructor runs here; board is ready to use
    Board();

    // A STATIC member function: it belongs to the class itself, not to any
    // particular Board, so you call it as  Board::fromFen("...")  without
    // having a Board first. It builds and returns a brand-new Board from FEN
    // text. A function like this, which creates objects, is often called a
    // "factory".
    // If the text is broken, it THROWS an exception (an error signal; see
    // board.cpp) of type std::invalid_argument explaining what's wrong.
    [[nodiscard]] static Board fromFen(std::string_view fen);

    // The reverse: describe this position as FEN text.
    [[nodiscard]] std::string toFen() const;

    // FEN without the last two numbers (move counters). Two positions with the
    // same key count as "the same position" for the draw-by-repetition rule.
    [[nodiscard]] std::string positionKey() const;

    // ---- Questions you can ask a Board ("getters") --------------------------
    // All are `const`: they promise not to change the Board. That matters,
    // because code that is only allowed to LOOK at a board (it has a
    // `const Board&`, see below) may call const functions only.
    //
    // These bodies are written right here in the header. Functions this
    // small are fine either way; putting them here lets the compiler paste
    // them directly where they're used (called INLINING), which is faster.

    // squares_[sq] reads element number sq of the squares_ array.
    [[nodiscard]] Piece pieceAt(Square sq) const noexcept { return squares_[sq]; }
    [[nodiscard]] Color sideToMove() const noexcept { return side_; }
    [[nodiscard]] std::uint8_t castlingRights() const noexcept { return castling_; }
    [[nodiscard]] Square enPassantSquare() const noexcept { return enPassant_; }
    [[nodiscard]] int halfmoveClock() const noexcept { return halfmoveClock_; }
    [[nodiscard]] int fullmoveNumber() const noexcept { return fullmoveNumber_; }
    [[nodiscard]] Square kingSquare(Color c) const noexcept {
        // kings_ has 2 entries: [0] = White's king, [1] = Black's king.
        // Color is an enum class (not a number), so we convert it to use it
        // as a position.
        return kings_[static_cast<std::size_t>(c)];
    }

    // Is square `sq` attacked by any piece of colour `by`?
    [[nodiscard]] bool isSquareAttacked(Square sq, Color by) const;

    // Is the player whose turn it is currently in check?
    [[nodiscard]] bool inCheck() const;

    // ---- Changing the board -------------------------------------------------
    // These are NOT const: they modify the Board.
    //
    // `const Move& move` is the first REFERENCE parameter we've used in
    // detail, so here it is:
    //
    //   REFERENCES
    //   Normally, when you pass a value to a function, the function gets its
    //   own COPY. Changing the copy doesn't affect the original.
    //   Writing `&` after the type makes the parameter a REFERENCE: another
    //   name for the caller's original variable. No copy is made.
    //     void f(Move m)         f gets a copy (fine for small things)
    //     void f(Move& m)        f works on the caller's actual Move and CAN change it
    //     void f(const Move& m)  f sees the caller's actual Move but CANNOT change it
    //   `const Type&` is the usual way to pass anything bigger than a few
    //   numbers: no copying cost, and a guarantee it won't be modified.
    //   A reference must always refer to something real; it can never be
    //   "empty". (POINTERS, written Type*, are the other way to refer to
    //   something; they can be empty. This project avoids them. See LEARNING.md.)
    //
    // makeMove plays a move. It trusts that the move is valid for this
    // position; MoveGenerator only ever produces valid ones.
    // It also remembers what it needs to reverse the move, so unmakeMove()
    // can take back moves one at a time, most recent first. The search uses
    // this to try millions of moves on ONE Board without copying it each time.
    void makeMove(const Move& move);  // `void` = returns nothing
    void unmakeMove();
    [[nodiscard]] bool canUnmake() const noexcept { return !history_.empty(); }

    // Returns a picture of the board as text, ready to print.
    // `bool unicode = true` is a DEFAULT ARGUMENT: callers may leave it out,
    // in which case it's true. So render() and render(true) do the same thing.
    [[nodiscard]] std::string render(bool unicode = true) const;

private:  // ---- only Board's own member functions can use what follows -------

    // A struct defined INSIDE the class: its full name is Board::UndoInfo, and
    // since it's private nobody outside Board can use it.
    // makeMove saves one of these for each move: everything a move destroys
    // that unmakeMove needs to put back.
    struct UndoInfo {
        Move move;
        Piece captured;           // what was taken (NoPiece if nothing)
        std::uint8_t castling;
        Square enPassant;
        int halfmoveClock;
    };

    // A private helper member function, used by the constructor and fromFen.
    void loadFen(std::string_view fen);

    // ---- The data. By convention, private member names end in _ so you can
    // tell them apart from local variables at a glance.

    // std::array<Piece, 64>: exactly 64 Pieces in a row, one per square,
    // indexed 0..63 like the diagram in types.hpp. `{}` starts them all as
    // default Pieces (empty).
    std::array<Piece, 64> squares_{};

    // Where each king is, so "is my king attacked?" doesn't have to search.
    std::array<Square, 2> kings_{NoSquare, NoSquare};

    Color side_ = Color::White;              // whose turn it is
    std::uint8_t castling_ = castling::None; // which castling moves are still allowed
    Square enPassant_ = NoSquare;            // square a pawn may capture en passant onto
    int halfmoveClock_ = 0;                  // moves since last capture/pawn move (50-move rule)
    int fullmoveNumber_ = 1;                 // starts at 1, +1 after each Black move

    // std::vector<UndoInfo>: a list of UndoInfo that GROWS as moves are made.
    // Unlike std::array, a vector's size can change while the program runs.
    // Its elements are stored in memory the vector requests from the system
    // (the HEAP) and gives back by itself, see below.
    std::vector<UndoInfo> history_;
};

// -----------------------------------------------------------------------------
// WHO CLEANS UP THE MEMORY? (RAII)
// -----------------------------------------------------------------------------
// A std::vector requests memory as it grows. That memory must be given back
// when the vector is no longer needed, or the program "leaks" it.
// In C++ every object can have a DESTRUCTOR: a function that runs
// automatically the moment the object stops existing (for a local variable,
// at the closing } of its block). std::vector's destructor gives its memory
// back. This pattern, "an object owns a resource and its destructor
// releases it", is called RAII (Resource Acquisition Is Initialization).
//
// Because Board is built only from members that already clean up after
// themselves, the compiler automatically writes Board's destructor, and
// its COPY behaviour too: `Board b2 = b1;` makes a full, independent copy,
// vector and all. We write none of that ourselves (the "rule of zero").
// Notice the project never asks for or frees memory by hand.

}  // namespace chess
