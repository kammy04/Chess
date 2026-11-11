#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "chess/board.hpp"
#include "chess/move_generator.hpp"
#include "test_util.hpp"

namespace chess {
namespace {

using test::sq;

bool contains(const std::vector<Move>& moves, std::string_view uci) {
    return std::any_of(moves.begin(), moves.end(), [&](const Move& m) { return m.toUci() == uci; });
}

// ---- Perft: the definitive move-generator test -------------------------------
// A parameterised test runs the same body for every row of the table below.

struct PerftCase {
    std::string_view name;
    std::string_view fen;
    int depth;
    std::uint64_t expected;
};

class Perft : public ::testing::TestWithParam<PerftCase> {};

TEST_P(Perft, MatchesReferenceCount) {
    const PerftCase& c = GetParam();
    Board board = Board::fromFen(c.fen);
    EXPECT_EQ(MoveGenerator::perft(board, c.depth), c.expected);
    EXPECT_EQ(board.toFen(), c.fen);  // perft must leave the board unchanged
}

INSTANTIATE_TEST_SUITE_P(
    KnownPositions, Perft,
    ::testing::Values(
        PerftCase{"start_d1", test::StartFen, 1, 20},
        PerftCase{"start_d2", test::StartFen, 2, 400},
        PerftCase{"start_d3", test::StartFen, 3, 8'902},
        PerftCase{"start_d4", test::StartFen, 4, 197'281},
        PerftCase{"kiwipete_d1", test::Kiwipete, 1, 48},
        PerftCase{"kiwipete_d2", test::Kiwipete, 2, 2'039},
        PerftCase{"kiwipete_d3", test::Kiwipete, 3, 97'862},
        PerftCase{"pos3_d1", test::Position3, 1, 14},
        PerftCase{"pos3_d2", test::Position3, 2, 191},
        PerftCase{"pos3_d3", test::Position3, 3, 2'812},
        PerftCase{"pos3_d4", test::Position3, 4, 43'238},
        PerftCase{"pos4_d1", test::Position4, 1, 6},
        PerftCase{"pos4_d2", test::Position4, 2, 264},
        PerftCase{"pos4_d3", test::Position4, 3, 9'467},
        PerftCase{"pos5_d1", test::Position5, 1, 44},
        PerftCase{"pos5_d2", test::Position5, 2, 1'486},
        PerftCase{"pos5_d3", test::Position5, 3, 62'379},
        PerftCase{"pos6_d1", test::Position6, 1, 46},
        PerftCase{"pos6_d2", test::Position6, 2, 2'079},
        PerftCase{"pos6_d3", test::Position6, 3, 89'890}),
    [](const ::testing::TestParamInfo<PerftCase>& info) { return std::string(info.param.name); });

// Millions of nodes: too slow for every build. Run with
//   ./chess_tests --gtest_also_run_disabled_tests --gtest_filter='*Deep*'
TEST(PerftDeep, DISABLED_ReferenceCountsAtDepth5) {
    Board start;
    EXPECT_EQ(MoveGenerator::perft(start, 5), 4'865'609U);
    Board kiwipete = Board::fromFen(test::Kiwipete);
    EXPECT_EQ(MoveGenerator::perft(kiwipete, 4), 4'085'603U);
    Board pos3 = Board::fromFen(test::Position3);
    EXPECT_EQ(MoveGenerator::perft(pos3, 5), 674'624U);
}

TEST(Perft, DepthZeroIsOneNode) {
    Board board;
    EXPECT_EQ(MoveGenerator::perft(board, 0), 1U);
}

TEST(Perft, DivideSumsToPerft) {
    Board board = Board::fromFen(test::Kiwipete);
    const auto divide = MoveGenerator::perftDivide(board, 2);
    EXPECT_EQ(divide.size(), 48U);
    std::uint64_t total = 0;
    for (const auto& [move, count] : divide) total += count;
    EXPECT_EQ(total, 2'039U);
}

// ---- Targeted rule tests -----------------------------------------------------

TEST(MoveGenerator, StartingPositionMoves) {
    Board board;
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_EQ(moves.size(), 20U);
    EXPECT_TRUE(contains(moves, "e2e4"));
    EXPECT_TRUE(contains(moves, "g1f3"));
    EXPECT_FALSE(contains(moves, "e1e2"));
}

TEST(MoveGenerator, CheckmatedSideHasNoMoves) {
    // Fool's mate.
    Board board = Board::fromFen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(board.inCheck());
    EXPECT_TRUE(MoveGenerator::generateLegal(board).empty());
}

TEST(MoveGenerator, PinnedPieceCannotLeaveTheLine) {
    // White knight e2 is pinned to the king by the rook on e8.
    Board board = Board::fromFen("4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_TRUE(std::none_of(moves.begin(), moves.end(), [](const Move& m) { return m.from == 12; }));
}

TEST(MoveGenerator, MustRespondToCheck) {
    // Rook on a1 checks the king on e1; only king moves off the rank or a block are legal.
    Board board = Board::fromFen("4k3/8/8/8/8/8/3B4/r3K3 w - - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_TRUE(contains(moves, "d2c1"));  // block
    EXPECT_TRUE(contains(moves, "e1e2"));
    EXPECT_FALSE(contains(moves, "d2e3"));  // ignores the check
    EXPECT_FALSE(contains(moves, "e1f1"));  // still on the rank
}

TEST(MoveGenerator, CannotCastleThroughOutOfOrIntoCheck) {
    // Black bishop on c4 covers f1: kingside castling would pass through check.
    Board through = Board::fromFen("4k3/8/8/8/2b5/8/8/R3K2R w KQ - 0 1");
    auto moves = MoveGenerator::generateLegal(through);
    EXPECT_FALSE(contains(moves, "e1g1"));
    EXPECT_TRUE(contains(moves, "e1c1"));

    // King in check from the rook on e8: no castling at all.
    Board outOf = Board::fromFen("4r1k1/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    moves = MoveGenerator::generateLegal(outOf);
    EXPECT_FALSE(contains(moves, "e1g1"));
    EXPECT_FALSE(contains(moves, "e1c1"));

    // Rook on g8 covers g1 (the king's destination).
    Board into = Board::fromFen("4k1r1/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    moves = MoveGenerator::generateLegal(into);
    EXPECT_FALSE(contains(moves, "e1g1"));
    EXPECT_TRUE(contains(moves, "e1c1"));
}

TEST(MoveGenerator, QueensideCastlingNeedsB1Empty) {
    Board board = Board::fromFen("4k3/8/8/8/8/8/8/RN2K3 w Q - 0 1");
    EXPECT_FALSE(contains(MoveGenerator::generateLegal(board), "e1c1"));
}

TEST(MoveGenerator, QueensideCastlingAllowedWhenOnlyB1Attacked) {
    // b1 may be attacked: the king never crosses it.
    Board board = Board::fromFen("1r2k3/8/8/8/8/8/8/R3K3 w Q - 0 1");
    EXPECT_TRUE(contains(MoveGenerator::generateLegal(board), "e1c1"));
}

TEST(MoveGenerator, BlackCastling) {
    Board board = Board::fromFen("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_TRUE(contains(moves, "e8g8"));
    EXPECT_TRUE(contains(moves, "e8c8"));
}

TEST(MoveGenerator, EnPassantAvailableOnlyImmediately) {
    Board board = Board::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    const auto ep = std::find_if(moves.begin(), moves.end(), [](const Move& m) { return m.toUci() == "e5d6"; });
    ASSERT_NE(ep, moves.end());
    EXPECT_EQ(ep->flag, MoveFlag::EnPassant);

    Board later = Board::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - - 0 1");
    EXPECT_FALSE(contains(MoveGenerator::generateLegal(later), "e5d6"));
}

TEST(MoveGenerator, EnPassantThatExposesKingIsIllegal) {
    // Capturing c6 en passant removes both pawns from rank 5, exposing the king
    // on a5 to the rook on h5. A classic move-generator bug.
    Board board = Board::fromFen("8/8/8/KPp4r/8/8/8/7k w - c6 0 1");
    EXPECT_FALSE(contains(MoveGenerator::generateLegal(board), "b5c6"));
}

TEST(MoveGenerator, PromotionsGenerateFourPieces) {
    Board board = Board::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    for (const char* uci : {"a7a8q", "a7a8r", "a7a8b", "a7a8n"}) {
        EXPECT_TRUE(contains(moves, uci)) << uci;
    }
    EXPECT_FALSE(contains(moves, "a7a8"));
}

TEST(MoveGenerator, BlackPawnMovesDownward) {
    Board board = Board::fromFen("4k3/4p3/8/8/8/8/8/4K3 b - - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_TRUE(contains(moves, "e7e6"));
    EXPECT_TRUE(contains(moves, "e7e5"));
}

TEST(MoveGenerator, BlockedPawnCannotPush) {
    Board board = Board::fromFen("4k3/8/8/8/8/4n3/4P3/4K3 w - - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_FALSE(contains(moves, "e2e3"));
    EXPECT_FALSE(contains(moves, "e2e4"));
}

TEST(MoveGenerator, SlidersStopAtPieces) {
    // The C version let the queen jump over one piece. Not here.
    // Queen a1, black pawn c3 on the diagonal, own pawn a3 on the file.
    Board board = Board::fromFen("4k3/8/8/8/8/P1p5/8/Q3K3 w - - 0 1");
    const auto moves = MoveGenerator::generateLegal(board);
    EXPECT_TRUE(contains(moves, "a1b2"));
    EXPECT_TRUE(contains(moves, "a1c3"));   // capture the first enemy piece...
    EXPECT_FALSE(contains(moves, "a1d4"));  // ...but never slide past it
    EXPECT_TRUE(contains(moves, "a1a2"));
    EXPECT_FALSE(contains(moves, "a1a3"));  // own piece: no capture
    EXPECT_FALSE(contains(moves, "a1a4"));
}

TEST(MoveGenerator, CannotCaptureOwnPieces) {
    Board board;
    const auto moves = MoveGenerator::generatePseudoLegal(board);
    for (const Move& m : moves) {
        EXPECT_TRUE(board.pieceAt(m.to).isEmpty()) << m;  // nothing to capture in the start position
    }
}

TEST(MoveGenerator, PseudoLegalIncludesMovesIntoCheck) {
    Board board = Board::fromFen("4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1");
    const auto pseudo = MoveGenerator::generatePseudoLegal(board);
    const auto legal = MoveGenerator::generateLegal(board);
    EXPECT_GT(pseudo.size(), legal.size());
}

}  // namespace
}  // namespace chess
