#include <gtest/gtest.h>

#include <sstream>

#include "chess/move.hpp"
#include "chess/types.hpp"
#include "test_util.hpp"

namespace chess {
namespace {

TEST(Types, SquareNumberingIsLittleEndianRankFile) {
    EXPECT_EQ(test::sq("a1"), 0);
    EXPECT_EQ(test::sq("h1"), 7);
    EXPECT_EQ(test::sq("a2"), 8);
    EXPECT_EQ(test::sq("e4"), 28);
    EXPECT_EQ(test::sq("h8"), 63);
    EXPECT_EQ(fileOf(test::sq("e4")), 4);
    EXPECT_EQ(rankOf(test::sq("e4")), 3);
}

TEST(Types, SquareStringRoundTrip) {
    for (Square s = 0; s < 64; ++s) {
        EXPECT_EQ(parseSquare(squareToString(s)), s);
    }
    EXPECT_EQ(squareToString(NoSquare), "-");
    EXPECT_EQ(squareToString(64), "-");
}

TEST(Types, ParseSquareRejectsBadInput) {
    for (const char* bad : {"", "e", "e44", "i1", "a9", "a0", "E4", "44"}) {
        EXPECT_FALSE(parseSquare(bad).has_value()) << bad;
    }
}

TEST(Types, OppositeColor) {
    EXPECT_EQ(opposite(Color::White), Color::Black);
    EXPECT_EQ(opposite(Color::Black), Color::White);
}

TEST(Types, PieceCharRoundTrip) {
    for (const char c : std::string_view("PNBRQKpnbrqk")) {
        const auto piece = pieceFromChar(c);
        ASSERT_TRUE(piece.has_value()) << c;
        EXPECT_EQ(pieceToChar(*piece), c);
    }
    EXPECT_EQ(pieceToChar(NoPiece), '.');
    EXPECT_FALSE(pieceFromChar('x').has_value());
    EXPECT_FALSE(pieceFromChar('1').has_value());
}

TEST(Types, PieceSymbols) {
    EXPECT_EQ(pieceSymbol({PieceType::Knight, Color::White}, true), "♘");
    EXPECT_EQ(pieceSymbol({PieceType::Knight, Color::Black}, true), "♞");
    EXPECT_EQ(pieceSymbol({PieceType::Queen, Color::White}, false), "Q");
    EXPECT_EQ(pieceSymbol({PieceType::Queen, Color::Black}, false), "q");
    EXPECT_EQ(pieceSymbol(NoPiece, true), " ");
}

TEST(Types, PieceEqualityAndPredicates) {
    const Piece whiteRook{PieceType::Rook, Color::White};
    EXPECT_TRUE(whiteRook.is(Color::White, PieceType::Rook));
    EXPECT_FALSE(whiteRook.is(Color::Black, PieceType::Rook));
    EXPECT_FALSE(whiteRook.isEmpty());
    EXPECT_TRUE(NoPiece.isEmpty());
    EXPECT_NE(whiteRook, (Piece{PieceType::Rook, Color::Black}));
}

TEST(Move, UciNotation) {
    EXPECT_EQ((Move{test::sq("e2"), test::sq("e4")}).toUci(), "e2e4");
    EXPECT_EQ((Move{test::sq("e7"), test::sq("e8"), PieceType::Knight}).toUci(), "e7e8n");
    EXPECT_EQ(Move{}.toUci(), "0000");
    EXPECT_TRUE(Move{}.isNull());
}

TEST(Move, Predicates) {
    const Move castle{test::sq("e1"), test::sq("g1"), PieceType::None, MoveFlag::CastleKingside};
    EXPECT_TRUE(castle.isCastle());
    EXPECT_FALSE(castle.isPromotion());
    EXPECT_TRUE((Move{0, 8, PieceType::Queen}).isPromotion());
}

TEST(Move, StreamOperator) {
    std::ostringstream out;
    out << Move{test::sq("g1"), test::sq("f3")};
    EXPECT_EQ(out.str(), "g1f3");
}

}  // namespace
}  // namespace chess
