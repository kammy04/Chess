// =============================================================================
// board_test.cpp: tests for Board: FEN reading/writing, make/unmake, attacks
// =============================================================================
// (How GoogleTest works is explained at the top of types_test.cpp.)
// `using test::sq;` lets us write sq("e4") for the square number of e4.

#include <gtest/gtest.h>

#include <stdexcept>

#include "chess/board.hpp"
#include "chess/move_generator.hpp"
#include "test_util.hpp"

namespace chess {
namespace {

using test::sq;

TEST(Board, DefaultIsStartingPosition) {
    const Board board;
    EXPECT_EQ(board.toFen(), test::StartFen);
    EXPECT_EQ(board.sideToMove(), Color::White);
    EXPECT_EQ(board.castlingRights(), castling::All);
    EXPECT_EQ(board.enPassantSquare(), NoSquare);
    EXPECT_EQ(board.kingSquare(Color::White), sq("e1"));
    EXPECT_EQ(board.kingSquare(Color::Black), sq("e8"));
    EXPECT_TRUE(board.pieceAt(sq("d1")).is(Color::White, PieceType::Queen));  // queen starts on her own colour
    EXPECT_FALSE(board.canUnmake());
}

TEST(Board, FenRoundTrip) {
    for (const auto fen : {test::StartFen, test::Kiwipete, test::Position3, test::Position4,
                           test::Position5, test::Position6}) {
        EXPECT_EQ(Board::fromFen(fen).toFen(), fen);
    }
}

TEST(Board, FenCountersAreOptional) {
    const Board board = Board::fromFen("4k3/8/8/8/8/8/8/4K3 w - -");
    EXPECT_EQ(board.halfmoveClock(), 0);
    EXPECT_EQ(board.fullmoveNumber(), 1);
}

TEST(Board, FenWithEnPassantSquare) {
    const Board board = Board::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    EXPECT_EQ(board.enPassantSquare(), sq("d6"));
}

TEST(Board, InvalidFenThrows) {
    const char* bad[] = {
        "",                                                          // no fields
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq",        // 3 fields
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1",           // 7 ranks
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/8 w KQkq - 0 1",// 9 ranks
        "rnbqkbnr/ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",   // short rank
        "rnbqkbnrr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // long rank
        "rnbqkbn1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // piece after a full rank
        "rnbqkbnx/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  // unknown piece
        "4k3/8/8/8/8/8/8/8 w - - 0 1",                               // no white king
        "4k3/8/8/8/8/8/8/3KK3 w - - 0 1",                            // two white kings
        "P3k3/8/8/8/8/8/8/4K3 w - - 0 1",                            // pawn on rank 8
        "4k3/8/8/8/8/8/8/4K3 x - - 0 1",                             // bad side
        "4k3/8/8/8/8/8/8/4K3 w X - 0 1",                             // bad castling
        "4k3/8/8/8/8/8/8/4K3 w - e9 0 1",                            // bad ep square
        "4k3/8/8/8/8/8/8/4K3 w - e3 0 1",                            // ep on wrong rank
        "4k3/8/8/8/8/8/8/4K3 w - e6 0 1",                            // ep with no pawn to capture
        "4k3/8/8/8/8/8/8/4K3 w - - -1 1",                            // negative counter
        "4k3/8/8/8/8/8/8/4K3 w - - x 1",                             // non-numeric counter
        "4k3/8/8/8/8/8/8/4K3 w - - 0 0",                             // fullmove 0
        "4k3/8/8/8/8/8/8/4R1K1 w - - 0 1",                           // black (not to move) in check
    };
    for (const char* fen : bad) {
        EXPECT_THROW((void)Board::fromFen(fen), std::invalid_argument) << fen;
    }
}

TEST(Board, CastlingRightsWithoutPiecesAreDropped) {
    // FEN claims all rights, but the white rooks and black kingside rook are missing.
    const Board board = Board::fromFen("r3k3/8/8/8/8/8/8/4K3 w KQkq - 0 1");
    EXPECT_EQ(board.castlingRights(), castling::BlackQueenside);
}

TEST(Board, DoublePawnPushSetsEnPassantSquare) {
    Board board;
    board.makeMove({sq("e2"), sq("e4"), PieceType::None, MoveFlag::DoublePawnPush});
    EXPECT_EQ(board.enPassantSquare(), sq("e3"));
    EXPECT_EQ(board.sideToMove(), Color::Black);
    board.makeMove({sq("g8"), sq("f6")});
    EXPECT_EQ(board.enPassantSquare(), NoSquare);  // the right expires after one move
}

TEST(Board, EnPassantCaptureRemovesPawnAndUnmakeRestores) {
    Board board = Board::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    const std::string before = board.toFen();
    board.makeMove({sq("e5"), sq("d6"), PieceType::None, MoveFlag::EnPassant});
    EXPECT_TRUE(board.pieceAt(sq("d6")).is(Color::White, PieceType::Pawn));
    EXPECT_TRUE(board.pieceAt(sq("d5")).isEmpty());
    EXPECT_TRUE(board.pieceAt(sq("e5")).isEmpty());
    board.unmakeMove();
    EXPECT_EQ(board.toFen(), before);
}

TEST(Board, CastlingMovesRookAndUnmakeRestores) {
    const auto fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    Board board = Board::fromFen(fen);

    board.makeMove({sq("e1"), sq("g1"), PieceType::None, MoveFlag::CastleKingside});
    EXPECT_TRUE(board.pieceAt(sq("g1")).is(Color::White, PieceType::King));
    EXPECT_TRUE(board.pieceAt(sq("f1")).is(Color::White, PieceType::Rook));
    EXPECT_TRUE(board.pieceAt(sq("h1")).isEmpty());
    EXPECT_EQ(board.kingSquare(Color::White), sq("g1"));
    EXPECT_EQ(board.castlingRights(), castling::BlackKingside | castling::BlackQueenside);

    board.makeMove({sq("e8"), sq("c8"), PieceType::None, MoveFlag::CastleQueenside});
    EXPECT_TRUE(board.pieceAt(sq("c8")).is(Color::Black, PieceType::King));
    EXPECT_TRUE(board.pieceAt(sq("d8")).is(Color::Black, PieceType::Rook));
    EXPECT_TRUE(board.pieceAt(sq("a8")).isEmpty());
    EXPECT_EQ(board.castlingRights(), castling::None);

    board.unmakeMove();
    board.unmakeMove();
    EXPECT_EQ(board.toFen(), fen);
}

TEST(Board, PromotionAndUnmakeRestoresPawn) {
    const auto fen = "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1";
    Board board = Board::fromFen(fen);
    board.makeMove({sq("a7"), sq("b8"), PieceType::Knight});  // capture-promotion
    EXPECT_TRUE(board.pieceAt(sq("b8")).is(Color::White, PieceType::Knight));
    EXPECT_TRUE(board.pieceAt(sq("a7")).isEmpty());
    board.unmakeMove();
    EXPECT_EQ(board.toFen(), fen);
    EXPECT_TRUE(board.pieceAt(sq("b8")).is(Color::Black, PieceType::Rook));
}

TEST(Board, CastlingRightsLostByRookMovesAndCaptures) {
    Board board = Board::fromFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    board.makeMove({sq("a1"), sq("a2")});  // queenside rook moves
    EXPECT_EQ(board.castlingRights(), castling::WhiteKingside | castling::BlackKingside | castling::BlackQueenside);
    board.makeMove({sq("e8"), sq("d8")});  // black king moves: both black rights gone
    EXPECT_EQ(board.castlingRights(), castling::WhiteKingside);
    board.makeMove({sq("h1"), sq("h8")});  // white rook captures on h8
    EXPECT_EQ(board.castlingRights(), castling::None);
}

TEST(Board, MoveCounters) {
    Board board;
    board.makeMove({sq("g1"), sq("f3")});
    EXPECT_EQ(board.halfmoveClock(), 1);
    EXPECT_EQ(board.fullmoveNumber(), 1);
    board.makeMove({sq("g8"), sq("f6")});
    EXPECT_EQ(board.halfmoveClock(), 2);
    EXPECT_EQ(board.fullmoveNumber(), 2);  // increments after Black moves
    board.makeMove({sq("e2"), sq("e4"), PieceType::None, MoveFlag::DoublePawnPush});
    EXPECT_EQ(board.halfmoveClock(), 0);  // pawn move resets the fifty-move counter
    board.unmakeMove();
    board.unmakeMove();
    EXPECT_EQ(board.halfmoveClock(), 1);
    EXPECT_EQ(board.fullmoveNumber(), 1);
}

TEST(Board, CaptureResetsHalfmoveClock) {
    Board board = Board::fromFen("4k3/8/8/3r4/8/8/8/3RK3 w - - 7 20");
    board.makeMove({sq("d1"), sq("d5")});
    EXPECT_EQ(board.halfmoveClock(), 0);
}

// Make + unmake of every legal move in many positions must restore the exact
// FEN. If unmake forgets anything (ep square, castling, a captured piece),
// this test finds it.
TEST(Board, MakeUnmakeRestoresEveryPosition) {
    for (const auto fen : {test::StartFen, test::Kiwipete, test::Position3, test::Position4,
                           test::Position5, test::Position6}) {
        Board board = Board::fromFen(fen);
        for (const Move& move : MoveGenerator::generateLegal(board)) {
            board.makeMove(move);
            board.unmakeMove();
            ASSERT_EQ(board.toFen(), fen) << "after " << move;
        }
    }
}

TEST(Board, AttacksByEachPieceType) {
    const Board board = Board::fromFen("4k3/8/8/3n4/8/2b5/3P4/R3K3 w - - 0 1");
    EXPECT_TRUE(board.isSquareAttacked(sq("c3"), Color::White));   // pawn d2 attacks c3
    EXPECT_TRUE(board.isSquareAttacked(sq("e3"), Color::White));   // pawn d2 attacks e3
    EXPECT_FALSE(board.isSquareAttacked(sq("d3"), Color::White));  // pawns don't attack forward
    EXPECT_TRUE(board.isSquareAttacked(sq("a8"), Color::White));   // rook a1 up the file
    EXPECT_TRUE(board.isSquareAttacked(sq("d1"), Color::White));   // rook along rank (and king)
    EXPECT_TRUE(board.isSquareAttacked(sq("f2"), Color::White));   // king
    EXPECT_TRUE(board.isSquareAttacked(sq("e3"), Color::Black));   // knight d5
    EXPECT_TRUE(board.isSquareAttacked(sq("d2"), Color::Black));   // bishop c3
    EXPECT_FALSE(board.isSquareAttacked(sq("e1"), Color::Black));  // bishop blocked by d2 pawn
    EXPECT_TRUE(board.isSquareAttacked(sq("d7"), Color::Black));   // king e8
    EXPECT_FALSE(board.inCheck());
}

TEST(Board, BlackPawnAttacksDownward) {
    const Board board = Board::fromFen("4k3/8/8/3p4/8/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(board.isSquareAttacked(sq("c4"), Color::Black));
    EXPECT_TRUE(board.isSquareAttacked(sq("e4"), Color::Black));
    EXPECT_FALSE(board.isSquareAttacked(sq("c6"), Color::Black));
}

TEST(Board, InCheck) {
    EXPECT_TRUE(Board::fromFen("4k3/8/8/8/8/8/8/q3K3 w - - 0 1").inCheck());
    EXPECT_TRUE(Board::fromFen("4k3/8/8/7B/8/8/8/4K3 b - - 0 1").inCheck());
    EXPECT_FALSE(Board::fromFen("q3k3/8/8/8/8/8/8/4K3 b - - 0 1").inCheck());
}

TEST(Board, RenderShowsPiecesInBothStyles) {
    const Board board;
    const std::string unicode = board.render(true);
    EXPECT_NE(unicode.find("♔"), std::string::npos);
    EXPECT_NE(unicode.find("♚"), std::string::npos);
    EXPECT_NE(unicode.find("┌"), std::string::npos);
    const std::string ascii = board.render(false);
    EXPECT_NE(ascii.find("| K |"), std::string::npos);
    EXPECT_NE(ascii.find("| k |"), std::string::npos);
    EXPECT_EQ(ascii.find("♔"), std::string::npos);
}

TEST(Board, CopiesAreIndependent) {
    Board original;
    Board copy = original;  // value semantics: a deep, independent copy
    copy.makeMove({sq("g1"), sq("f3")});
    EXPECT_EQ(original.toFen(), test::StartFen);
    EXPECT_NE(copy.toFen(), test::StartFen);
}

}  // namespace
}  // namespace chess
