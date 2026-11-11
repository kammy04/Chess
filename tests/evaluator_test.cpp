#include <gtest/gtest.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "chess/board.hpp"
#include "chess/evaluator.hpp"
#include "test_util.hpp"

namespace chess {
namespace {

TEST(Evaluator, StartingPositionIsBalanced) {
    const Evaluator eval;
    const Board board;
    EXPECT_EQ(eval.evaluateWhite(board), 0);
    EXPECT_EQ(eval.evaluate(board), 0);
}

TEST(Evaluator, PieceValues) {
    EXPECT_EQ(Evaluator::pieceValue(PieceType::Pawn), 100);
    EXPECT_LT(Evaluator::pieceValue(PieceType::Knight), Evaluator::pieceValue(PieceType::Bishop));
    EXPECT_LT(Evaluator::pieceValue(PieceType::Bishop), Evaluator::pieceValue(PieceType::Rook));
    EXPECT_LT(Evaluator::pieceValue(PieceType::Rook), Evaluator::pieceValue(PieceType::Queen));
    EXPECT_EQ(Evaluator::pieceValue(PieceType::King), 0);
    EXPECT_EQ(Evaluator::pieceValue(PieceType::None), 0);
}

TEST(Evaluator, ExtraMaterialFavoursItsOwner) {
    const Evaluator eval;
    EXPECT_GT(eval.evaluateWhite(Board::fromFen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1")), 800);
    EXPECT_LT(eval.evaluateWhite(Board::fromFen("3qk3/8/8/8/8/8/8/4K3 w - - 0 1")), -800);
}

TEST(Evaluator, ScoreIsFromSideToMovesPerspective) {
    const Evaluator eval;
    const Board whiteToMove = Board::fromFen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    const Board blackToMove = Board::fromFen("4k3/8/8/8/8/8/8/3QK3 b - - 0 1");
    EXPECT_EQ(eval.evaluate(whiteToMove), eval.evaluateWhite(whiteToMove));
    EXPECT_EQ(eval.evaluate(blackToMove), -eval.evaluateWhite(blackToMove));
}

// A position and its colour-mirrored twin must score the same for the side to
// move. This catches table-indexing mistakes for Black.
std::string mirrorFen(std::string_view fen) {
    std::istringstream in{std::string(fen)};
    std::string placement, side, rights, ep;
    in >> placement >> side >> rights >> ep;

    // Reverse the rank order and swap the case (colour) of every piece.
    std::vector<std::string> ranks;
    std::istringstream rankStream(placement);
    for (std::string rank; std::getline(rankStream, rank, '/');) ranks.push_back(rank);
    std::string out;
    for (auto it = ranks.rbegin(); it != ranks.rend(); ++it) {
        for (char c : *it) {
            const auto uc = static_cast<unsigned char>(c);
            out += static_cast<char>(std::isupper(uc) ? std::tolower(uc) : std::toupper(uc));
        }
        if (it + 1 != ranks.rend()) out += '/';
    }
    std::string swappedRights;
    for (char c : rights) {
        const auto uc = static_cast<unsigned char>(c);
        swappedRights += c == '-' ? c : static_cast<char>(std::isupper(uc) ? std::tolower(uc) : std::toupper(uc));
    }
    if (ep != "-") ep[1] = static_cast<char>('1' + ('8' - ep[1]));
    return out + (side == "w" ? " b " : " w ") + swappedRights + ' ' + ep + " 0 1";
}

TEST(Evaluator, MirrorFenHelper) {
    EXPECT_EQ(mirrorFen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"), "4k3/4p3/8/8/8/8/8/4K3 b - - 0 1");
}

TEST(Evaluator, MirroredPositionsAreSymmetric) {
    const Evaluator eval;
    for (const auto fen : {test::StartFen, test::Kiwipete, test::Position3, test::Position4,
                           test::Position5, test::Position6}) {
        const Board original = Board::fromFen(fen);
        const Board mirrored = Board::fromFen(mirrorFen(fen));
        EXPECT_EQ(eval.evaluate(original), eval.evaluate(mirrored)) << fen;
    }
}

TEST(Evaluator, CentralKnightBeatsRimKnight) {
    const Evaluator eval;
    const int centre = eval.evaluateWhite(Board::fromFen("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1"));
    const int rim = eval.evaluateWhite(Board::fromFen("4k3/8/8/8/N7/8/8/4K3 w - - 0 1"));
    EXPECT_GT(centre, rim);
}

TEST(Evaluator, AdvancedPawnIsWorthMore) {
    const Evaluator eval;
    const int advanced = eval.evaluateWhite(Board::fromFen("4k3/4P3/8/8/8/8/8/4K3 w - - 0 1"));
    const int home = eval.evaluateWhite(Board::fromFen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"));
    EXPECT_GT(advanced, home);
}

}  // namespace
}  // namespace chess
