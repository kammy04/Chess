// =============================================================================
// types.cpp: the instructions (definitions) for the functions DECLARED in
//            include/chess/types.hpp
// =============================================================================
// Read types.hpp first. This is the first .cpp (source) file, so it introduces
// how a source file is laid out.

// Every .cpp file includes its own header first, so the compiler can check
// that what we define here matches what we promised there.
// Quotes " " mean "our own project file". CMakeLists.txt tells the compiler to
// look in the include/ folder, which is why the path starts with chess/.
#include "chess/types.hpp"

#include <array>   // std::array: a fixed-size list
#include <cctype>  // character helpers: std::toupper, std::tolower, std::isupper

// We re-open the same namespace. Everything defined inside is chess::...,
// matching the declarations in the header.
namespace chess {

// DEFINITION of squareToString. The first line matches the declaration in the
// header exactly, but now there's a body in { }.
std::string squareToString(Square sq) {
    // `if (condition) { ... }` runs the block only when the condition is true.
    // `!` means NOT, so this reads "if sq is NOT a valid square".
    if (!isValidSquare(sq)) {
        return "-";
    }
    // Characters are stored as numbers, so we can do maths on them:
    // 'a' + 4 is 'e', and '1' + 3 is '4'. The result of adding is an `int`,
    // though, so we convert it back to `char`.
    //   static_cast<char>(x)  is an explicit CONVERSION: "turn x into a char".
    //   It's written out loud so conversions are never accidental.
    // `return {a, b};` builds the std::string from those two characters.
    return {static_cast<char>('a' + fileOf(sq)), static_cast<char>('1' + rankOf(sq))};
}

std::optional<Square> parseSquare(std::string_view text) {
    // .size() is a member function of string_view: how many characters it has.
    // `!=` means "not equal".
    if (text.size() != 2) {
        return std::nullopt;  // std::nullopt is "nothing": the empty optional
    }
    // text[0] is the FIRST character, text[1] the second (counting starts at 0).
    // 'e' - 'a' = 4 and '4' - '1' = 3, turning letters/digits into 0-based numbers.
    // `const int file = ...;` creates a VARIABLE named file of type int.
    // `const` means it can't be changed after this line; we use const
    // whenever a value shouldn't change, so the compiler catches mistakes.
    const int file = text[0] - 'a';
    const int rank = text[1] - '1';
    if (!onBoard(file, rank)) {
        return std::nullopt;
    }
    return makeSquare(file, rank);  // a plain Square is automatically wrapped in the optional
}

char pieceToChar(Piece piece) {
    // A string_view over a fixed piece of text. constexpr: built at compile time.
    // Position 0 is '.', 1 is 'p', 2 is 'n', ... matching the PieceType order.
    constexpr std::string_view letters = ".pnbrqk";

    // piece.type is a PieceType (an enum class), which is not a number, so we
    // must convert it explicitly to use it as a position.
    // std::size_t is the standard unsigned type for sizes and positions.
    const char c = letters[static_cast<std::size_t>(piece.type)];

    // `||` means OR. Empty squares and black pieces stay lowercase.
    if (piece.isEmpty() || piece.color == Color::Black) {
        return c;
    }
    // std::toupper turns 'n' into 'N'. Its quirk: it wants an unsigned char and
    // returns an int, hence the two conversions.
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

std::optional<Piece> pieceFromChar(char c) {
    constexpr std::string_view letters = "pnbrqk";

    // `auto` means "compiler, work out the type from the right-hand side".
    // Here it becomes `unsigned char`. Use it when the type is obvious or long.
    const auto uc = static_cast<unsigned char>(c);

    // .find(x) searches the text and returns the POSITION of x, or the special
    // value std::string_view::npos ("no position") if x isn't there.
    const auto pos = letters.find(static_cast<char>(std::tolower(uc)));
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    const Color color = std::isupper(uc) ? Color::White : Color::Black;

    // Piece{type, color} builds a Piece with those field values, in order.
    // pos + 1 because letters starts at 'p' (PieceType::Pawn is 1, not 0).
    return Piece{static_cast<PieceType>(pos + 1), color};
}

std::string_view pieceSymbol(Piece piece, bool unicode) {
    // std::array<Type, Count> is a FIXED-SIZE list: here 7 pieces of text.
    // It knows its own size and is stored directly, with no extra memory to manage.
    //   static     inside a function: created ONCE and kept for the whole
    //              program, instead of being rebuilt on every call
    //   constexpr  built at compile time
    // { " ", "♙", ... } lists the 7 elements; position 0 is PieceType::None.
    static constexpr std::array<std::string_view, 7> whiteUnicode{" ", "♙", "♘", "♗", "♖", "♕", "♔"};
    static constexpr std::array<std::string_view, 7> blackUnicode{" ", "♟", "♞", "♝", "♜", "♛", "♚"};
    static constexpr std::array<std::string_view, 7> whiteAscii{" ", "P", "N", "B", "R", "Q", "K"};
    static constexpr std::array<std::string_view, 7> blackAscii{" ", "p", "n", "b", "r", "q", "k"};

    const auto index = static_cast<std::size_t>(piece.type);
    if (piece.color == Color::White) {
        // name[index] reads the element at that position (0-based).
        return unicode ? whiteUnicode[index] : whiteAscii[index];
    }
    return unicode ? blackUnicode[index] : blackAscii[index];
}

}  // namespace chess
