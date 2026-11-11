// =============================================================================
// types.hpp: the basic building blocks every other file uses
// =============================================================================
//
// START HERE. This is the first file to read. Every C++ idea is explained the
// first time it appears; later files point back here instead of repeating.
//
// Lines starting with // are COMMENTS. The compiler ignores them completely;
// they exist only for humans. (/* like this */ is the other comment style.)
//
// WHAT IS A ".hpp" FILE?
// C++ code is split into two kinds of files:
//   * HEADER files (.hpp) DECLARE things: "these types and functions exist,
//     and here is what they look like". Think of a table of contents.
//   * SOURCE files (.cpp) DEFINE things: the actual instructions inside each
//     function. Think of the chapters.
// Any file that wants to use something from here writes
// #include "chess/types.hpp" at the top. The matching instructions for the
// functions declared below live in src/types.cpp.

// `#pragma once` means: "if this header is included more than once while
// compiling one .cpp file, only read it the first time." Without it the same
// types would be defined twice, which is a compile error. Every header in this
// project starts with it.
#pragma once

// `#include <...>` pulls in a header from the C++ STANDARD LIBRARY, the
// toolbox of ready-made types and functions that comes with every C++
// compiler. Angle brackets < > mean "a library header"; quotes " " mean "one
// of our own files". Each include gives us specific tools:
#include <cstdint>      // exact-size whole-number types, e.g. std::uint8_t
#include <optional>     // std::optional: holds "a value, or nothing"
#include <string>       // std::string: a piece of text that can grow and change
#include <string_view>  // std::string_view: a read-only "window" onto some text

// A NAMESPACE is a named box for names. Everything between `namespace chess {`
// and its closing `}` really has the full name chess::Something (for example
// chess::Color). This stops our names from clashing with names in other code.
// The `std::` you see everywhere is the standard library's namespace: `std::`
// means "look inside the box called std". `::` is the SCOPE operator; read it
// as "the thing inside".
namespace chess {

// -----------------------------------------------------------------------------
// Color
// -----------------------------------------------------------------------------
// An ENUM (short for enumeration) is a type whose value must be one of a fixed
// list of named options. `enum class` is the strict, modern version:
//   * you always write the full name: Color::White, never just White
//   * it is NOT treated as a number, so you can't accidentally do maths on a
//     colour or compare a Color with a PieceType. The compiler refuses.
// `: std::uint8_t` picks how the value is stored in memory: an unsigned
// ("never negative") 8-bit integer, i.e. one byte, range 0 to 255.
// Behind the scenes White is stored as 0 and Black as 1.
enum class Color : std::uint8_t { White, Black };

// A FUNCTION: a named, reusable block of instructions. Read the first line
// word by word:
//   constexpr   "may be calculated while COMPILING if the input is already
//               known then". It's a speed bonus, and also allowed at normal run time.
//   Color       the RETURN TYPE: the kind of value this function hands back
//   opposite    the function's name
//   (Color c)   the PARAMETERS: inputs. Here one input named c of type Color.
//   noexcept    a promise that this function never throws an exception
//               (exceptions are C++'s error signals; see board.cpp)
// The part between { and } is the BODY: the instructions that run.
// `return` ends the function and hands back a value.
// `==` asks "are these equal?" and gives true or false. (A single `=` means
// something else entirely: ASSIGN, i.e. store a value.)
// `condition ? a : b` is the TERNARY operator: "if condition is true then a,
// otherwise b".
// Usage elsewhere:  Color them = opposite(Color::White);   // them is Black
constexpr Color opposite(Color c) noexcept {
    return c == Color::White ? Color::Black : Color::White;
}

// The kinds of chess piece. The ORDER matters: None=0, Pawn=1, ..., King=6.
// Other files use those numbers as positions in lookup lists (e.g. "the
// symbol for piece type 2"), so don't reorder this list.
enum class PieceType : std::uint8_t { None, Pawn, Knight, Bishop, Rook, Queen, King };

// -----------------------------------------------------------------------------
// Piece
// -----------------------------------------------------------------------------
// A STRUCT bundles several values into one new type. A Piece is "what kind"
// plus "whose": for example {Knight, White}.
struct Piece {
    // These two lines declare MEMBER VARIABLES (also called FIELDS): data that
    // every Piece carries. The form is  Type name = defaultValue;
    // The `= ...` part is a DEFAULT MEMBER INITIALIZER: a Piece created
    // without saying otherwise is an empty square (type None).
    // Every statement in C++ ends with a semicolon ;
    PieceType type = PieceType::None;
    Color color = Color::White;

    // MEMBER FUNCTIONS belong to the struct and can read its fields directly.
    // You call them with a dot:  if (piece.isEmpty()) { ... }
    //
    //   [[nodiscard]]  an ATTRIBUTE, an extra instruction to the compiler:
    //                  "warn if someone calls this and ignores the answer".
    //                  Calling isEmpty() and throwing the result away is
    //                  always a mistake, so we ask to be warned.
    //   bool           the return type: true or false
    //   const          (AFTER the parentheses) a promise that this function
    //                  only LOOKS at the Piece and never changes it
    //
    // The whole body fits on one line, which is fine for tiny functions.
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return type == PieceType::None; }

    // `&&` means AND: both sides must be true. (`||` means OR; `!` means NOT.)
    // Usage:  piece.is(Color::White, PieceType::King)
    [[nodiscard]] constexpr bool is(Color c, PieceType t) const noexcept {
        return type == t && color == c;
    }

    // OPERATOR OVERLOADING: teaching C++ what `==` means for our own type.
    // Normally `pieceA == pieceB` would not compile, because C++ doesn't know
    // how to compare Pieces. `= default` says "compiler, write it for me:
    // two Pieces are equal when ALL their fields are equal" (needs C++20).
    // `const Piece&` is the parameter type: a REFERENCE to another Piece.
    // References are explained properly in board.hpp; for now read it as
    // "the other Piece, looked at directly rather than copied".
    bool operator==(const Piece&) const = default;
};  // note: a struct definition ends with `};` (brace AND semicolon)

// A named CONSTANT: a value that can never change.
//   inline constexpr  "a compile-time constant that lives in a header and is
//                      safely shared by every file that includes it"
//   Piece             its type
//   NoPiece           its name
//   {}                create it with all the default values, i.e. an empty square
inline constexpr Piece NoPiece{};

// -----------------------------------------------------------------------------
// Squares
// -----------------------------------------------------------------------------
// A TYPE ALIAS: `using Square = int;` gives `int` a second, more meaningful
// name. `int` is C++'s everyday whole-number type (it can be negative).
// A Square is just a number from 0 to 63:
//
//      a  b  c  d  e  f  g  h
//   8  56 57 58 59 60 61 62 63
//   7  48 49 50 51 52 53 54 55
//   ...
//   2   8  9 10 11 12 13 14 15
//   1   0  1  2  3  4  5  6  7
//
// so a1 = 0, h1 = 7, a2 = 8, e4 = 28, h8 = 63.
using Square = int;

// -1 is used to mean "no square" (e.g. "there is no en-passant square").
inline constexpr Square NoSquare = -1;

// Small helper functions for converting between a square number and its
// file (column a to h, as 0 to 7) and rank (row 1 to 8, as 0 to 7).
//   %  is REMAINDER: 28 % 8 = 4 (28 divided by 8 is 3, remainder 4) -> e-file
//   /  between two ints is WHOLE-NUMBER division: 28 / 8 = 3 (the .5 is dropped) -> rank 4
//   *  is multiply, + add, - subtract
constexpr int fileOf(Square sq) noexcept { return sq % 8; }  // 0 = a-file ... 7 = h-file
constexpr int rankOf(Square sq) noexcept { return sq / 8; }  // 0 = rank 1 ... 7 = rank 8
constexpr Square makeSquare(int file, int rank) noexcept { return rank * 8 + file; }

// `>=` greater-or-equal, `<` less-than. True only if both file and rank are 0..7.
constexpr bool onBoard(int file, int rank) noexcept {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}
constexpr bool isValidSquare(Square sq) noexcept { return sq >= 0 && sq < 64; }

// -----------------------------------------------------------------------------
// Converting to and from text
// -----------------------------------------------------------------------------
// These are DECLARATIONS ONLY: name, inputs and return type, ending in `;`
// with no body. The bodies (definitions) are in src/types.cpp. Splitting them
// means other files only need to know HOW to call a function, not how it works.

// 28 -> "e4".  std::string is the standard library's text type.
[[nodiscard]] std::string squareToString(Square sq);

// "e4" -> 28, but text like "z9" is not a square. std::optional<Square> means
// "maybe a Square, maybe nothing". The angle brackets <Square> fill in WHAT
// it might hold (this is called a TEMPLATE argument; std::optional works for
// any type). The caller must check before using the value:
//     std::optional<Square> s = parseSquare("e4");
//     if (s) { use *s; }   // `if (s)` asks "is there a value?"; `*s` gets it out
// std::string_view is a cheap, read-only way to receive text: it points at
// characters that already exist instead of copying them.
[[nodiscard]] std::optional<Square> parseSquare(std::string_view text);

// The one-letter codes used in FEN (a standard text format for chess
// positions, see LEARNING.md): uppercase = White, lowercase = Black.
//   P/p pawn  N/n knight  B/b bishop  R/r rook  Q/q queen  K/k king
// `char` is the type for ONE character, written in single quotes: 'P'.
// (Double quotes "P" make a piece of text, which is a different type.)
// An empty square is written '.'.
[[nodiscard]] char pieceToChar(Piece piece);
[[nodiscard]] std::optional<Piece> pieceFromChar(char c);

// The symbol to print for a piece: "♘" when unicode is true, "N" otherwise.
[[nodiscard]] std::string_view pieceSymbol(Piece piece, bool unicode);

}  // namespace chess   <- closes the box opened at the top
