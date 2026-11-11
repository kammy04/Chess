#include "chess/game.hpp"

#include <algorithm>

#include "chess/move_generator.hpp"

namespace chess {

std::string_view describe(GameResult result) {
    switch (result) {
        case GameResult::Ongoing: return "game in progress";
        case GameResult::WhiteWinsByCheckmate: return "checkmate - White wins";
        case GameResult::BlackWinsByCheckmate: return "checkmate - Black wins";
        case GameResult::Stalemate: return "draw by stalemate";
        case GameResult::FiftyMoveRule: return "draw by the fifty-move rule";
        case GameResult::ThreefoldRepetition: return "draw by threefold repetition";
        case GameResult::InsufficientMaterial: return "draw by insufficient material";
    }
    return "unknown";
}

Game::Game() : Game(Board{}) {}  // delegating constructor

// Member initialiser list: members are constructed directly with these values
// instead of default-constructed and then assigned.
Game::Game(const Board& start) : start_(start), board_(start), positionKeys_{start.positionKey()} {}

std::vector<Move> Game::legalMoves() const {
    Board scratch = board_;  // generateLegal needs a mutable board; we promised const
    return MoveGenerator::generateLegal(scratch);
}

std::optional<Move> Game::parseMove(std::string_view text) const {
    if (text.size() != 4 && text.size() != 5) return std::nullopt;
    const auto from = parseSquare(text.substr(0, 2));
    const auto to = parseSquare(text.substr(2, 2));
    if (!from || !to) return std::nullopt;

    PieceType promotion = PieceType::None;
    if (text.size() == 5) {
        const auto piece = pieceFromChar(text[4]);
        if (!piece || piece->type == PieceType::Pawn || piece->type == PieceType::King) {
            return std::nullopt;
        }
        promotion = piece->type;
    }

    for (const Move& move : legalMoves()) {
        if (move.from != *from || move.to != *to) continue;
        const PieceType wanted = (move.isPromotion() && promotion == PieceType::None)
                                     ? PieceType::Queen
                                     : promotion;
        if (move.promotion == wanted) return move;
    }
    return std::nullopt;
}

bool Game::play(const Move& move) {
    if (isOver()) return false;
    const std::vector<Move> legal = legalMoves();
    if (std::find(legal.begin(), legal.end(), move) == legal.end()) return false;

    board_.makeMove(move);
    moves_.push_back(move);
    positionKeys_.push_back(board_.positionKey());
    return true;
}

bool Game::play(std::string_view text) {
    const auto move = parseMove(text);
    return move && play(*move);
}

bool Game::undo() {
    if (moves_.empty()) return false;
    board_.unmakeMove();
    moves_.pop_back();
    positionKeys_.pop_back();
    return true;
}

void Game::reset() {
    board_ = start_;
    moves_.clear();
    positionKeys_.assign(1, start_.positionKey());
}

GameResult Game::result() const {
    if (legalMoves().empty()) {
        if (!board_.inCheck()) return GameResult::Stalemate;
        return board_.sideToMove() == Color::White ? GameResult::BlackWinsByCheckmate
                                                   : GameResult::WhiteWinsByCheckmate;
    }
    if (board_.halfmoveClock() >= 100) return GameResult::FiftyMoveRule;
    const auto repeats = std::count(positionKeys_.begin(), positionKeys_.end(), positionKeys_.back());
    if (repeats >= 3) return GameResult::ThreefoldRepetition;
    if (insufficientMaterial()) return GameResult::InsufficientMaterial;
    return GameResult::Ongoing;
}

// Draw when neither side can possibly mate: K v K, K+minor v K, or only
// bishops that all stand on the same colour of square.
bool Game::insufficientMaterial() const {
    int knights = 0;
    int bishops = 0;
    bool bishopOnLight = false;
    bool bishopOnDark = false;
    for (Square sq = 0; sq < 64; ++sq) {
        const Piece p = board_.pieceAt(sq);
        switch (p.type) {
            case PieceType::Pawn:
            case PieceType::Rook:
            case PieceType::Queen: return false;
            case PieceType::Knight: ++knights; break;
            case PieceType::Bishop:
                ++bishops;
                ((fileOf(sq) + rankOf(sq)) % 2 == 0 ? bishopOnDark : bishopOnLight) = true;
                break;
            default: break;
        }
    }
    if (knights + bishops <= 1) return true;
    return knights == 0 && !(bishopOnLight && bishopOnDark);
}

}  // namespace chess
