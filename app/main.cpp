// Terminal front end: play chess against a friend or the engine, or run the
// perft / benchmark tools used to measure the engine in Phases 2 and 3.

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "chess/game.hpp"
#include "chess/move_generator.hpp"
#include "chess/search.hpp"

namespace {

using namespace chess;

enum class Mode { HumanVsHuman, HumanVsEngine, EngineVsEngine };

struct Options {
    Mode mode = Mode::HumanVsEngine;
    Color humanColor = Color::White;
    int depth = 4;
    bool unicode = true;
};

void printUsage() {
    std::cout << "Usage:\n"
                 "  chess [--mode pvp|pve|eve] [--color white|black] [--depth N] [--ascii]\n"
                 "  chess --perft <depth> [FEN]     count move-tree leaves (move generator check)\n"
                 "  chess --bench [depth]           search benchmark positions (default depth 4)\n";
}

void printHelp() {
    std::cout << "Commands:\n"
                 "  e2e4, e7e8q   make a move (coordinate notation; promotion defaults to queen)\n"
                 "  moves         list legal moves\n"
                 "  undo          take back a move (vs engine: takes back your move and its reply)\n"
                 "  reset         start a new game\n"
                 "  history       show moves played\n"
                 "  fen           print the position as FEN\n"
                 "  load <FEN>    start from a custom position\n"
                 "  help, quit\n";
}

std::optional<int> parseInt(std::string_view text) {
    try {
        std::size_t used = 0;
        const int value = std::stoi(std::string(text), &used);
        if (used == text.size()) return value;
    } catch (const std::exception&) {
    }
    return std::nullopt;
}

std::string formatScore(int score) {
    if (isMateScore(score)) return "mate " + std::to_string(mateInMoves(score));
    std::ostringstream out;
    out << std::showpos << std::fixed << std::setprecision(2) << score / 100.0;
    return out.str();
}

void printStats(const SearchResult& r) {
    const double ms = static_cast<double>(r.elapsed.count()) / 1000.0;
    const double nps = ms > 0 ? static_cast<double>(r.nodes) / (ms / 1000.0) : 0.0;
    std::cout << "depth " << r.depth << "  score " << formatScore(r.score) << "  nodes " << r.nodes
              << "  time " << std::fixed << std::setprecision(1) << ms << " ms  nps "
              << static_cast<long long>(nps) << '\n';
}

bool isEngineTurn(const Options& opt, Color side) {
    switch (opt.mode) {
        case Mode::HumanVsHuman: return false;
        case Mode::EngineVsEngine: return true;
        case Mode::HumanVsEngine: return side != opt.humanColor;
    }
    return false;
}

int runGame(const Options& opt) {
    Game game;
    Search search;
    std::cout << "Terminal Chess (C++ engine)\n";
    printHelp();

    while (true) {
        std::cout << '\n' << game.board().render(opt.unicode);
        const GameResult result = game.result();
        if (result != GameResult::Ongoing) {
            std::cout << "Game over: " << describe(result) << '\n';
            if (opt.mode == Mode::EngineVsEngine) return 0;
            std::cout << "Type 'undo', 'reset' or 'quit'.\n";
        } else {
            std::cout << (game.board().sideToMove() == Color::White ? "White" : "Black") << " to move"
                      << (game.board().inCheck() ? " - check!" : "") << '\n';
        }

        if (result == GameResult::Ongoing && isEngineTurn(opt, game.board().sideToMove())) {
            Board scratch = game.board();
            const SearchResult r = search.findBestMove(scratch, opt.depth);
            game.play(r.bestMove);
            std::cout << "Engine plays " << r.bestMove << "   ";
            printStats(r);
            continue;
        }

        std::cout << "> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {  // EOF (Ctrl-D): exit cleanly, unlike the C version
            std::cout << "\nGoodbye.\n";
            return 0;
        }
        std::istringstream words(line);
        std::string command;
        words >> command;

        if (command.empty()) {
            continue;
        } else if (command == "quit" || command == "q" || command == "exit") {
            std::cout << "Goodbye.\n";
            return 0;
        } else if (command == "help") {
            printHelp();
        } else if (command == "moves") {
            for (const Move& m : game.legalMoves()) std::cout << m << ' ';
            std::cout << '\n';
        } else if (command == "history") {
            int ply = 0;
            for (const Move& m : game.moveHistory()) {
                if (ply % 2 == 0) std::cout << (ply / 2 + 1) << ". ";
                std::cout << m << ' ';
                ++ply;
            }
            std::cout << '\n';
        } else if (command == "undo") {
            bool undone = game.undo();
            // Against the engine, keep undoing until it is the human's turn again.
            while (undone && isEngineTurn(opt, game.board().sideToMove()) && game.undo()) {
            }
            if (!undone) std::cout << "Nothing to undo.\n";
        } else if (command == "reset") {
            game.reset();
        } else if (command == "fen") {
            std::cout << game.board().toFen() << '\n';
        } else if (command == "load") {
            std::string fen;
            std::getline(words >> std::ws, fen);
            try {
                game = Game(Board::fromFen(fen));
            } catch (const std::invalid_argument& e) {
                std::cout << e.what() << '\n';
            }
        } else if (!game.play(command)) {
            std::cout << "Illegal move or unknown command: '" << command << "' (type 'help')\n";
        }
    }
}

int runPerft(int depth, const std::string& fen) {
    Board board = Board::fromFen(fen);
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t total = 0;
    for (const auto& [move, count] : MoveGenerator::perftDivide(board, depth)) {
        std::cout << move << ": " << count << '\n';
        total += count;
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
    std::cout << "\nNodes searched: " << total << "  (" << ms << " ms)\n";
    return 0;
}

// Fixed positions searched to a fixed depth. Run this before and after every
// Phase 3 change: nodes and time should drop while depth goes up.
int runBench(int depth) {
    struct Position {
        const char* name;
        const char* fen;
    };
    const std::vector<Position> positions{
        {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
        {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"},
        {"italian", "r1bqk1nr/pppp1ppp/2n5/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"},
        {"middlegame", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"},
        {"endgame", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"},
    };

    Search search;
    std::uint64_t totalNodes = 0;
    std::chrono::microseconds totalTime{0};
    std::cout << std::left << std::setw(12) << "position" << std::setw(8) << "best"
              << "stats\n";
    for (const auto& [name, fen] : positions) {
        Board board = Board::fromFen(fen);
        const SearchResult r = search.findBestMove(board, depth);
        std::cout << std::left << std::setw(12) << name << std::setw(8) << r.bestMove.toUci();
        printStats(r);
        totalNodes += r.nodes;
        totalTime += r.elapsed;
    }
    std::cout << "\nTotal: " << totalNodes << " nodes, " << totalTime.count() / 1000 << " ms\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    Options opt;

    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string_view arg = args[i];
            const auto next = [&]() -> std::string_view {
                if (i + 1 >= args.size()) throw std::invalid_argument("missing value after " + std::string(arg));
                return args[++i];
            };

            if (arg == "--perft") {
                const auto depth = parseInt(next());
                if (!depth || *depth < 1) throw std::invalid_argument("--perft needs a positive depth");
                std::string fen;
                for (++i; i < args.size(); ++i) fen += std::string(args[i]) + ' ';
                return runPerft(*depth, fen.empty() ? std::string(Board::StartFen) : fen);
            }
            if (arg == "--bench") {
                const auto depth = i + 1 < args.size() ? parseInt(args[i + 1]) : std::optional<int>{4};
                if (!depth || *depth < 1) throw std::invalid_argument("--bench needs a positive depth");
                return runBench(*depth);
            }
            if (arg == "--mode") {
                const std::string_view mode = next();
                if (mode == "pvp") opt.mode = Mode::HumanVsHuman;
                else if (mode == "pve") opt.mode = Mode::HumanVsEngine;
                else if (mode == "eve") opt.mode = Mode::EngineVsEngine;
                else throw std::invalid_argument("unknown mode " + std::string(mode));
            } else if (arg == "--color") {
                const std::string_view color = next();
                if (color == "white") opt.humanColor = Color::White;
                else if (color == "black") opt.humanColor = Color::Black;
                else throw std::invalid_argument("unknown color " + std::string(color));
            } else if (arg == "--depth") {
                const auto depth = parseInt(next());
                if (!depth || *depth < 1 || *depth > 10) throw std::invalid_argument("--depth must be 1..10");
                opt.depth = *depth;
            } else if (arg == "--ascii") {
                opt.unicode = false;
            } else if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            } else {
                throw std::invalid_argument("unknown option " + std::string(arg));
            }
        }
        return runGame(opt);
    } catch (const std::invalid_argument& e) {
        std::cerr << "error: " << e.what() << "\n\n";
        printUsage();
        return EXIT_FAILURE;
    }
}
