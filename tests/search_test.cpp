// =============================================================================
// search_test.cpp: does the engine find good moves?
// =============================================================================
// Includes checkmate puzzles, and the most important test of all:
// AlphaBetaMatchesMinimaxWithFewerNodes, which shows that the faster
// algorithm gives exactly the same answer as the simple one.

#include <gtest/gtest.h>

#include "chess/board.hpp"
#include "chess/search.hpp"
#include "test_util.hpp"

namespace chess {
namespace {

TEST(Search, FindsBackRankMateInOne) {
    Board board = Board::fromFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    Search search;
    const SearchResult r = search.findBestMove(board, 2);
    EXPECT_EQ(r.bestMove.toUci(), "a1a8");
    EXPECT_EQ(r.score, MateScore - 1);
    EXPECT_TRUE(isMateScore(r.score));
    EXPECT_EQ(mateInMoves(r.score), 1);
}

TEST(Search, FindsMateInOneForBlack) {
    // Fool's mate: ...Qh4#
    Board board = Board::fromFen("rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2");
    Search search;
    const SearchResult r = search.findBestMove(board, 2);
    EXPECT_EQ(r.bestMove.toUci(), "d8h4");
    EXPECT_EQ(mateInMoves(r.score), 1);
}

TEST(Search, FindsMateInTwo) {
    // 1. Kb6 Kb8 2. Rh8#  -- the mating move is ply 3.
    // Depth must be 4, not 3: leaves (depth 0) return the static evaluation
    // without generating moves, so "no legal moves = mate" is only noticed one
    // ply deeper than the mating move. Same reason mate-in-one needs depth 2.
    Board board = Board::fromFen("k7/8/2K5/8/8/8/8/7R w - - 0 1");
    Search search;
    EXPECT_FALSE(isMateScore(search.findBestMove(board, 3).score));
    const SearchResult r = search.findBestMove(board, 4);
    EXPECT_EQ(r.score, MateScore - 3);
    EXPECT_EQ(mateInMoves(r.score), 2);
}

TEST(Search, PrefersTheFasterMate) {
    // Mate in one is available; a deeper search must not prefer a slower mate.
    Board board = Board::fromFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    Search search;
    EXPECT_EQ(search.findBestMove(board, 4).score, MateScore - 1);
}

TEST(Search, CapturesHangingQueen) {
    Board board = Board::fromFen("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");
    Search search;
    EXPECT_EQ(search.findBestMove(board, 2).bestMove.toUci(), "d1d5");
}

TEST(Search, DoesNotTakePoisonedPawn) {
    // Qxd5 wins a pawn but the black rook on d8 recaptures the queen.
    Board board = Board::fromFen("3rk3/8/8/3p4/8/8/8/3QK3 w - - 0 1");
    Search search;
    EXPECT_NE(search.findBestMove(board, 2).bestMove.toUci(), "d1d5");
}

TEST(Search, NoLegalMovesReturnsNullMove) {
    Board stalemate = Board::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    Search search;
    SearchResult r = search.findBestMove(stalemate, 3);
    EXPECT_TRUE(r.bestMove.isNull());
    EXPECT_EQ(r.score, 0);

    Board mated = Board::fromFen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    r = search.findBestMove(mated, 3);
    EXPECT_TRUE(r.bestMove.isNull());
    EXPECT_EQ(r.score, -MateScore);
}

TEST(Search, FiftyMoveRuleScoresDraw) {
    // White is a queen up, but any non-pawn, non-capture move hits the 100-ply limit.
    Board board = Board::fromFen("4k3/8/8/8/8/8/8/Q3K3 w - - 99 80");
    Search search;
    EXPECT_EQ(search.findBestMove(board, 2).score, 0);
}

TEST(Search, BoardIsRestoredAfterSearch) {
    Board board = Board::fromFen(test::Kiwipete);
    Search search;
    (void)search.findBestMove(board, 3);
    EXPECT_EQ(board.toFen(), test::Kiwipete);
}

TEST(Search, DepthBelowOneIsClampedToOne) {
    Board board;
    Search search;
    const SearchResult r = search.findBestMove(board, 0);
    EXPECT_EQ(r.depth, 1);
    EXPECT_FALSE(r.bestMove.isNull());
}

TEST(Search, IsDeterministic) {
    Board board = Board::fromFen(test::Position6);
    Search search;
    const SearchResult a = search.findBestMove(board, 3);
    const SearchResult b = search.findBestMove(board, 3);
    EXPECT_EQ(a.bestMove, b.bestMove);
    EXPECT_EQ(a.score, b.score);
    EXPECT_EQ(a.nodes, b.nodes);
}

// The key property of alpha-beta: exactly the same result as plain minimax,
// while visiting fewer nodes. If this ever fails, the pruning is wrong.
TEST(Search, AlphaBetaMatchesMinimaxWithFewerNodes) {
    for (const auto fen : {test::StartFen, test::Kiwipete, test::Position3, test::Position4,
                           test::Position5, test::Position6}) {
        Board board = Board::fromFen(fen);
        Search minimax(Search::Algorithm::Minimax);
        Search alphaBeta(Search::Algorithm::AlphaBeta);
        const SearchResult slow = minimax.findBestMove(board, 3);
        const SearchResult fast = alphaBeta.findBestMove(board, 3);
        EXPECT_EQ(fast.score, slow.score) << fen;
        EXPECT_EQ(fast.bestMove, slow.bestMove) << fen;
        EXPECT_LT(fast.nodes, slow.nodes) << fen;
    }
}

TEST(Search, MateScoreHelpers) {
    EXPECT_TRUE(isMateScore(MateScore - 5));
    EXPECT_TRUE(isMateScore(-MateScore + 2));
    EXPECT_FALSE(isMateScore(900));
    EXPECT_EQ(mateInMoves(MateScore - 1), 1);
    EXPECT_EQ(mateInMoves(MateScore - 3), 2);
    EXPECT_EQ(mateInMoves(-MateScore + 2), -1);
}

}  // namespace
}  // namespace chess
