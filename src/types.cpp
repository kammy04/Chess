#include "chess/types.hpp"

#include <array>
#include <cctype>

namespace chess {

std::string squareToString(Square sq) {
    if (!isValidSquare(sq)) {
        return "-";
    }
    return {static_cast<char>('a' + fileOf(sq)), static_cast<char>('1' + rankOf(sq))};
}

std::optional<Square> parseSquare(std::string_view text) {
    if (text.size() != 2) {
        return std::nullopt;
    }
    const int file = text[0] - 'a';
    const int rank = text[1] - '1';
    if (!onBoard(file, rank)) {
        return std::nullopt;
    }
    return makeSquare(file, rank);
}

char pieceToChar(Piece piece) {
    constexpr std::string_view letters = ".pnbrqk";  // indexed by PieceType
    const char c = letters[static_cast<std::size_t>(piece.type)];
    if (piece.isEmpty() || piece.color == Color::Black) {
        return c;
    }
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

std::optional<Piece> pieceFromChar(char c) {
    constexpr std::string_view letters = "pnbrqk";
    const auto uc = static_cast<unsigned char>(c);
    const auto pos = letters.find(static_cast<char>(std::tolower(uc)));
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    const Color color = std::isupper(uc) ? Color::White : Color::Black;
    return Piece{static_cast<PieceType>(pos + 1), color};
}

std::string_view pieceSymbol(Piece piece, bool unicode) {
    // Indexed by PieceType; std::array knows its own size (unlike a C array
    // that decays to a pointer) and works with range-for and .size().
    static constexpr std::array<std::string_view, 7> whiteUnicode{" ", "♙", "♘", "♗", "♖", "♕", "♔"};
    static constexpr std::array<std::string_view, 7> blackUnicode{" ", "♟", "♞", "♝", "♜", "♛", "♚"};
    static constexpr std::array<std::string_view, 7> whiteAscii{" ", "P", "N", "B", "R", "Q", "K"};
    static constexpr std::array<std::string_view, 7> blackAscii{" ", "p", "n", "b", "r", "q", "k"};

    const auto index = static_cast<std::size_t>(piece.type);
    if (piece.color == Color::White) {
        return unicode ? whiteUnicode[index] : whiteAscii[index];
    }
    return unicode ? blackUnicode[index] : blackAscii[index];
}

}  // namespace chess
