#include "chess/move.hpp"

namespace chess {

std::string Move::toUci() const {
    if (isNull()) {
        return "0000";
    }
    std::string text = squareToString(from) + squareToString(to);
    if (isPromotion()) {
        // UCI always uses lowercase for the promotion piece.
        text += pieceToChar(Piece{promotion, Color::Black});
    }
    return text;
}

std::ostream& operator<<(std::ostream& os, const Move& move) {
    return os << move.toUci();
}

}  // namespace chess
