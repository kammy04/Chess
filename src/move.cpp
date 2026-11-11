// =============================================================================
// move.cpp: definitions for include/chess/move.hpp
// =============================================================================

#include "chess/move.hpp"

namespace chess {

// `Move::toUci` means "the toUci that belongs to Move". When a member function
// is defined OUTSIDE its struct/class, you write ClassName:: before its name.
// The trailing `const` must match the declaration: this function doesn't
// change the Move.
std::string Move::toUci() const {
    // Inside a member function you can use the fields (from, to, ...) and
    // other members (isNull()) directly, without writing `move.` in front.
    if (isNull()) {
        return "0000";
    }
    // `+` joins two std::strings together: "e2" + "e4" is "e2e4".
    // We store the result in a variable called text. It's not const,
    // because we may add to it below.
    std::string text = squareToString(from) + squareToString(to);
    if (isPromotion()) {
        // `+=` means "add on to the end": text = text + ...
        // This notation always uses a lowercase promotion letter, and our
        // pieceToChar gives lowercase for Black, so we ask for a black piece.
        text += pieceToChar(Piece{promotion, Color::Black});
    }
    return text;
}

// Definition of the printing function declared in move.hpp.
//   std::ostream& os    the stream to print into, passed BY REFERENCE (&), so
//                       we write into the real stream, not a copy of it
//   const Move& move    the move, by reference too, and const: we only read it
// `os << move.toUci()` prints the text, and that expression gives back `os`,
// which we return so callers can keep chaining <<.
std::ostream& operator<<(std::ostream& os, const Move& move) {
    return os << move.toUci();
}

}  // namespace chess
