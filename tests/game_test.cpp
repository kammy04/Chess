// =============================================================================
// game_test.cpp: whole-game rules: checkmate, draws, undo, reset, input parsing
// =============================================================================
// Moves are typed as text ("e2e4"), exactly like a person playing.

#include <gtest/gtest.h>

#include <initializer_list>
#include <string_view>

#include "chess/game.hpp"
#include "test_util.hpp"

namespace chess {
namespace {

using test::sq;

void playAll(Game& game, std::initializer_list<std::string_view> moves) {
    for (const auto move : moves) {
        ASSERT_TRUE(game.play(move)) << "rejected " << move;
    }
}

TEST(Game, StartsOngoing) {
    const Game game;
    EXPECT_EQ(game.result(), GameResult::Ongoing);
    EXPECT_FALSE(game.isOver());
    EXPECT_EQ(game.legalMoves().size(), 20U);
    EXPECT_TRUE(game.moveHistory().empty());
}

TEST(Game, FoolsMate) {
    Game game;
    playAll(game, {"f2f3", "e7e5", "g2g4", "d8h4"});
    EXPECT_EQ(game.result(), GameResult::BlackWinsByCheckmate);
    EXPECT_TRUE(game.isOver());
    EXPECT_FALSE(game.play("e2e4"));  // no moves after the game ends
}

TEST(Game, ScholarsMate) {
    Game game;
    playAll(game, {"e2e4", "e7e5", "f1c4", "b8c6", "d1h5", "g8f6", "h5f7"});
    EXPECT_EQ(game.result(), GameResult::WhiteWinsByCheckmate);
}

TEST(Game, Stalemate) {
    const Game game(Board::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1"));
    EXPECT_EQ(game.result(), GameResult::Stalemate);
}

TEST(Game, ThreefoldRepetition) {
    Game game;
    playAll(game, {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1"});
    EXPECT_EQ(game.result(), GameResult::Ongoing);  // start position seen twice so far
    playAll(game, {"f6g8"});
    EXPECT_EQ(game.result(), GameResult::ThreefoldRepetition);
}

TEST(Game, FiftyMoveRule) {
    Game game(Board::fromFen("4k3/8/8/8/8/8/8/R3K3 w - - 99 60"));
    EXPECT_EQ(game.result(), GameResult::Ongoing);
    playAll(game, {"a1a2"});
    EXPECT_EQ(game.result(), GameResult::FiftyMoveRule);
}

TEST(Game, CheckmateBeatsFiftyMoveRule) {
    // The mating move is also the 100th half-move: checkmate takes precedence.
    Game game(Board::fromFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 99 60"));
    playAll(game, {"a1a8"});
    EXPECT_EQ(game.result(), GameResult::WhiteWinsByCheckmate);
}

TEST(Game, InsufficientMaterial) {
    for (const char* fen : {
             "4k3/8/8/8/8/8/8/4K3 w - - 0 1",     // K v K
             "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1",   // K+B v K
             "4k3/8/8/8/8/8/8/1N2K3 w - - 0 1",   // K+N v K
             "4kb2/8/8/8/8/8/8/2B1K3 w - - 0 1",  // bishops on c1 and f8: both dark squares
         }) {
        EXPECT_EQ(Game(Board::fromFen(fen)).result(), GameResult::InsufficientMaterial) << fen;
    }
}

TEST(Game, SufficientMaterial) {
    for (const char* fen : {
             "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",    // a pawn can promote
             "4k3/8/8/8/8/8/8/R3K3 w - - 0 1",     // rook
             "4k3/8/8/8/8/8/8/3QK3 w - - 0 1",     // queen
             "2b1k3/8/8/8/8/8/8/2B1K3 w - - 0 1",  // opposite-coloured bishops (c1 dark, c8 light)
             "4k3/8/8/8/8/8/8/1NB1K3 w - - 0 1",   // knight + bishop can mate
             "4k3/8/8/8/8/8/8/NN2K3 w - - 0 1",    // two knights: no forced mate, but mate is possible
         }) {
        EXPECT_EQ(Game(Board::fromFen(fen)).result(), GameResult::Ongoing) << fen;
    }
}

TEST(Game, RejectsIllegalAndMalformedMoves) {
    Game game;
    for (const char* bad : {"e2e5", "e7e5", "e1e2", "zz", "", "e2", "e2e4e", "e2e4x", "e2e4k", "e2e4p", "i2i4"}) {
        EXPECT_FALSE(game.play(bad)) << bad;
    }
    EXPECT_TRUE(game.moveHistory().empty());
    EXPECT_FALSE(game.play(Move{sq("e2"), sq("e5")}));
}

TEST(Game, PromotionDefaultsToQueen) {
    Game game(Board::fromFen("8/P6k/8/8/8/8/8/K7 w - - 0 1"));
    playAll(game, {"a7a8"});
    EXPECT_TRUE(game.board().pieceAt(sq("a8")).is(Color::White, PieceType::Queen));
}

TEST(Game, UnderPromotion) {
    Game game(Board::fromFen("8/P6k/8/8/8/8/8/K7 w - - 0 1"));
    playAll(game, {"a7a8n"});
    EXPECT_TRUE(game.board().pieceAt(sq("a8")).is(Color::White, PieceType::Knight));
}

TEST(Game, PromotionLetterOnNonPromotionIsRejected) {
    Game game;
    EXPECT_FALSE(game.play("e2e4q"));
}

TEST(Game, UndoAndReset) {
    Game game;
    EXPECT_FALSE(game.undo());
    playAll(game, {"e2e4", "e7e5", "g1f3"});
    EXPECT_EQ(game.moveHistory().size(), 3U);

    EXPECT_TRUE(game.undo());
    EXPECT_EQ(game.moveHistory().size(), 2U);
    EXPECT_EQ(game.board().sideToMove(), Color::White);
    EXPECT_TRUE(game.board().pieceAt(sq("g1")).is(Color::White, PieceType::Knight));

    game.reset();
    EXPECT_EQ(game.board().toFen(), test::StartFen);
    EXPECT_TRUE(game.moveHistory().empty());
    EXPECT_FALSE(game.undo());
}

TEST(Game, UndoRestoresRepetitionCount) {
    Game game;
    playAll(game, {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1", "f6g8"});
    ASSERT_EQ(game.result(), GameResult::ThreefoldRepetition);
    EXPECT_TRUE(game.undo());
    EXPECT_EQ(game.result(), GameResult::Ongoing);
}

TEST(Game, ResetReturnsToCustomStart) {
    const auto fen = "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1";
    Game game(Board::fromFen(fen));
    playAll(game, {"e2e4"});
    game.reset();
    EXPECT_EQ(game.board().toFen(), fen);
}

TEST(Game, DescribeResults) {
    EXPECT_EQ(describe(GameResult::Ongoing), "game in progress");
    EXPECT_EQ(describe(GameResult::WhiteWinsByCheckmate), "checkmate - White wins");
    EXPECT_EQ(describe(GameResult::BlackWinsByCheckmate), "checkmate - Black wins");
    EXPECT_EQ(describe(GameResult::Stalemate), "draw by stalemate");
    EXPECT_EQ(describe(GameResult::FiftyMoveRule), "draw by the fifty-move rule");
    EXPECT_EQ(describe(GameResult::ThreefoldRepetition), "draw by threefold repetition");
    EXPECT_EQ(describe(GameResult::InsufficientMaterial), "draw by insufficient material");
}

}  // namespace
}  // namespace chess
