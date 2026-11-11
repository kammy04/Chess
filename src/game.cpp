// =============================================================================
// game.cpp: definitions for include/chess/game.hpp
// =============================================================================
// New C++ ideas: delegating constructors, member initializer lists with
// several members, ITERATORS with the standard algorithms std::find and
// std::count, and how a const function can still use a function that needs
// to modify a board (by working on a copy).

#include "chess/game.hpp"

#include <algorithm>  // std::find, std::count

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

// A DELEGATING CONSTRUCTOR: `: Game(Board{})` hands the work to the other
// constructor, passing it a starting-position Board. No duplicated code.
Game::Game() : Game(Board{}) {}

// MEMBER INITIALIZER LIST (see Search in search.hpp): sets start_ and board_
// to copies of `start`, and positionKeys_ to a list holding one element (the
// starting position's key). The constructor body { } has nothing left to do.
Game::Game(const Board& start) : start_(start), board_(start), positionKeys_{start.positionKey()} {}

std::vector<Move> Game::legalMoves() const {
    // This function is const (it promised not to change the Game), but
    // generateLegal needs a board it can modify temporarily. Solution: make a
    // COPY. `Board scratch = board_;` is a full independent copy (see the RAII
    // note in board.hpp), so changing scratch can't affect board_.
    Board scratch = board_;
    return MoveGenerator::generateLegal(scratch);
}

std::optional<Move> Game::parseMove(std::string_view text) const {
    if (text.size() != 4 && text.size() != 5) return std::nullopt;

    // .substr(start, length) takes part of the text: substr(0, 2) of "e2e4" is "e2".
    const auto from = parseSquare(text.substr(0, 2));
    const auto to = parseSquare(text.substr(2, 2));
    if (!from || !to) return std::nullopt;

    PieceType promotion = PieceType::None;
    if (text.size() == 5) {
        const auto piece = pieceFromChar(text[4]);  // the 5th character, e.g. 'q'
        if (!piece || piece->type == PieceType::Pawn || piece->type == PieceType::King) {
            return std::nullopt;  // can't promote to a pawn or king
        }
        promotion = piece->type;
    }

    // Find the legal move that matches what was typed.
    for (const Move& move : legalMoves()) {
        if (move.from != *from || move.to != *to) continue;
        // A promotion typed without a letter means queen.
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

    // ITERATORS: a vector's .begin() is a marker pointing at its first
    // element and .end() a marker just PAST its last one. Standard algorithms
    // take such a pair to know which range to work on.
    // std::find(begin, end, value) looks for `value` and returns a marker to
    // it, or `end` if it's not there. So "== legal.end()" means "not found".
    if (std::find(legal.begin(), legal.end(), move) == legal.end()) return false;

    board_.makeMove(move);
    moves_.push_back(move);
    positionKeys_.push_back(board_.positionKey());
    return true;
}

bool Game::play(std::string_view text) {
    const auto move = parseMove(text);
    // An optional counts as true when it holds a value. Thanks to && short-
    // circuiting, play(*move) only runs if a move was found.
    return move && play(*move);
}

bool Game::undo() {
    if (moves_.empty()) return false;
    board_.unmakeMove();
    moves_.pop_back();         // remove the last element
    positionKeys_.pop_back();
    return true;
}

void Game::reset() {
    board_ = start_;   // ASSIGNMENT copies the whole Board, overwriting the current one
    moves_.clear();
    // .assign(count, value) replaces the contents with `count` copies of `value`.
    positionKeys_.assign(1, start_.positionKey());
}

GameResult Game::result() const {
    if (legalMoves().empty()) {
        if (!board_.inCheck()) return GameResult::Stalemate;
        // The player to move has been checkmated, so the OTHER player wins.
        return board_.sideToMove() == Color::White ? GameResult::BlackWinsByCheckmate
                                                   : GameResult::WhiteWinsByCheckmate;
    }
    if (board_.halfmoveClock() >= 100) return GameResult::FiftyMoveRule;

    // std::count(begin, end, value): how many elements equal value.
    // .back() is the newest key, i.e. the current position.
    const auto repeats = std::count(positionKeys_.begin(), positionKeys_.end(), positionKeys_.back());
    if (repeats >= 3) return GameResult::ThreefoldRepetition;
    if (insufficientMaterial()) return GameResult::InsufficientMaterial;
    return GameResult::Ongoing;
}

// Draw if nobody can possibly checkmate: just kings, king plus one bishop or
// knight, or only bishops that all stand on the same colour of square.
bool Game::insufficientMaterial() const {
    int knights = 0;
    int bishops = 0;
    bool bishopOnLight = false;
    bool bishopOnDark = false;
    for (Square sq = 0; sq < 64; ++sq) {
        const Piece p = board_.pieceAt(sq);
        switch (p.type) {
            // Several case labels in a row share the same code.
            case PieceType::Pawn:
            case PieceType::Rook:
            case PieceType::Queen: return false;  // enough to mate (or could become so)
            case PieceType::Knight: ++knights; break;
            case PieceType::Bishop:
                ++bishops;
                // (file + rank) even = dark square, odd = light square.
                // The ternary picks WHICH variable to set to true.
                ((fileOf(sq) + rankOf(sq)) % 2 == 0 ? bishopOnDark : bishopOnLight) = true;
                break;
            default: break;
        }
    }
    if (knights + bishops <= 1) return true;
    return knights == 0 && !(bishopOnLight && bishopOnDark);
}

}  // namespace chess
