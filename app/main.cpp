// =============================================================================
// main.cpp: the program you actually run: the terminal chess game
// =============================================================================
// Everything in include/ and src/ is the ENGINE (a library of reusable code).
// This file is the user-facing program built on top of it. It also has two
// measuring tools: --perft and --bench.
//
// New C++ ideas: the main() function, command-line arguments, reading
// keyboard input (std::getline, std::cin), printing (std::cout), formatting
// numbers, and catching exceptions with try/catch.

#include <chrono>
#include <cstdlib>    // EXIT_FAILURE: the standard "something went wrong" exit code
#include <iomanip>    // std::setw, std::setprecision: control how numbers are printed
#include <iostream>   // std::cout (print), std::cin (keyboard), std::cerr (print errors)
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "chess/game.hpp"
#include "chess/move_generator.hpp"
#include "chess/search.hpp"

namespace {  // helpers private to this file (see board.cpp)

// `using namespace chess;` means "inside this block, let me write Board
// instead of chess::Board". Handy in a file like this one; avoid it in
// headers, where it would spread to every file that includes them.
using namespace chess;

enum class Mode { HumanVsHuman, HumanVsEngine, EngineVsEngine };

// Settings chosen on the command line, with their default values.
struct Options {
    Mode mode = Mode::HumanVsEngine;
    Color humanColor = Color::White;
    int depth = 4;
    bool unicode = true;
};

void printUsage() {
    // Several "..." pieces of text written next to each other are joined
    // into one by the compiler, so a long message can span several lines.
    // "\n" inside text is a newline.
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

// Text -> int, or nothing if the text isn't a whole number.
std::optional<int> parseInt(std::string_view text) {
    // TRY / CATCH: run the code in `try { }`. If anything inside THROWS an
    // exception, jump straight to the matching `catch` block instead of
    // crashing. `const std::exception&` catches any standard exception.
    try {
        std::size_t used = 0;
        // std::stoi ("string to int") converts text to an int. It throws if
        // the text doesn't start with a number. `&used` passes the ADDRESS of
        // `used` (a pointer), so stoi can write back how many characters it read.
        const int value = std::stoi(std::string(text), &used);
        if (used == text.size()) return value;  // the WHOLE text was a number
    } catch (const std::exception&) {
        // Not a number: fall through to the "nothing" below.
    }
    return std::nullopt;
}

// Score -> readable text, e.g. "+0.50" or "mate 2".
std::string formatScore(int score) {
    if (isMateScore(score)) return "mate " + std::to_string(mateInMoves(score));
    std::ostringstream out;  // a stream that prints into a string (see board.cpp)
    // Stream MANIPULATORS change how the next numbers are printed:
    //   std::showpos           always show the sign (+ or -)
    //   std::fixed + std::setprecision(2)   exactly 2 digits after the point
    // `score / 100.0`: dividing by 100.0 (a DECIMAL number, type double)
    // keeps the fraction; dividing by 100 would drop it (whole-number division).
    out << std::showpos << std::fixed << std::setprecision(2) << score / 100.0;
    return out.str();
}

void printStats(const SearchResult& r) {
    // `double` is a decimal-number type. .count() gets the raw number out of
    // a std::chrono duration.
    const double ms = static_cast<double>(r.elapsed.count()) / 1000.0;
    const double nps = ms > 0 ? static_cast<double>(r.nodes) / (ms / 1000.0) : 0.0;  // nodes per second
    std::cout << "depth " << r.depth << "  score " << formatScore(r.score) << "  nodes " << r.nodes
              << "  time " << std::fixed << std::setprecision(1) << ms << " ms  nps "
              << static_cast<long long>(nps) << '\n';  // long long: a big whole-number type
}

bool isEngineTurn(const Options& opt, Color side) {
    switch (opt.mode) {
        case Mode::HumanVsHuman: return false;
        case Mode::EngineVsEngine: return true;
        case Mode::HumanVsEngine: return side != opt.humanColor;
    }
    return false;
}

// The interactive game loop.
int runGame(const Options& opt) {
    Game game;      // a new game (constructor runs)
    Search search;  // an engine with default settings
    std::cout << "Terminal Chess (C++ engine)\n";
    printHelp();

    // `while (true)` loops forever; we leave it with `return` from inside.
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
            // game.board() gives a const (read-only) Board, but the search needs
            // one it can modify while thinking, so we give it a copy.
            Board scratch = game.board();
            const SearchResult r = search.findBestMove(scratch, opt.depth);
            game.play(r.bestMove);
            std::cout << "Engine plays " << r.bestMove << "   ";  // uses operator<< from move.hpp
            printStats(r);
            continue;  // back to the top of the loop
        }

        // `<< std::flush` makes sure "> " appears on screen BEFORE we wait for typing.
        std::cout << "> " << std::flush;
        std::string line;
        // std::getline(std::cin, line) reads one whole line typed by the user
        // (std::cin is the keyboard) into `line`. It counts as false when there's
        // no more input (e.g. the user pressed Ctrl-D), and then we quit cleanly.
        if (!std::getline(std::cin, line)) {
            std::cout << "\nGoodbye.\n";
            return 0;
        }
        // Split the line into words: the first word is the command.
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
                if (ply % 2 == 0) std::cout << (ply / 2 + 1) << ". ";  // move number before White's move
                std::cout << m << ' ';
                ++ply;
            }
            std::cout << '\n';
        } else if (command == "undo") {
            bool undone = game.undo();
            // Against the engine, keep undoing until it's the human's turn again.
            // (The loop body is empty { }: all the work happens in the condition.)
            while (undone && isEngineTurn(opt, game.board().sideToMove()) && game.undo()) {
            }
            if (!undone) std::cout << "Nothing to undo.\n";
        } else if (command == "reset") {
            game.reset();
        } else if (command == "fen") {
            std::cout << game.board().toFen() << '\n';
        } else if (command == "load") {
            std::string fen;
            // `words >> std::ws` skips spaces; getline then takes the REST of the line.
            std::getline(words >> std::ws, fen);
            try {
                // Board::fromFen throws if the FEN is bad (see board.cpp).
                game = Game(Board::fromFen(fen));
            } catch (const std::invalid_argument& e) {
                // `e` is the caught exception; e.what() returns its message.
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
    // Each element of perftDivide's result is a std::pair; the structured
    // binding unpacks it into `move` and `count`.
    for (const auto& [move, count] : MoveGenerator::perftDivide(board, depth)) {
        std::cout << move << ": " << count << '\n';
        total += count;
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
    std::cout << "\nNodes searched: " << total << "  (" << ms << " ms)\n";
    return 0;
}

// Searches fixed positions to a fixed depth and prints speed statistics.
// Run it before and after every improvement to measure the difference.
int runBench(int depth) {
    // A struct declared inside a function, used only here.
    // `const char*` is a pointer to fixed text (the simplest form of text in C++).
    struct Position {
        const char* name;
        const char* fen;
    };
    // A vector filled with values right away, using braces.
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
    // std::left + std::setw(12): print the next value left-aligned in a column 12 characters wide.
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

// -----------------------------------------------------------------------------
// main: WHERE THE PROGRAM STARTS
// -----------------------------------------------------------------------------
// Every C++ program has exactly one function named main. When you run
// ./build/chess, the operating system calls main. Its return value is the
// program's EXIT CODE: 0 means success, anything else means failure.
//
// The two parameters hold the COMMAND-LINE ARGUMENTS, the words typed after
// the program name. For `./build/chess --depth 5`:
//   argc = 3  (argument count: "./build/chess", "--depth", "5")
//   argv      (argument values) a list of pointers to those texts
// `char** argv` means "pointer to a list of pointers to characters", the
// old-fashioned low-level way to pass a list of texts. We immediately
// convert it into a friendly std::vector of string_views.
int main(int argc, char** argv) {
    // Build a vector from a RANGE: from argv + 1 (skip the program's own
    // name) up to argv + argc (the end). (Pointer + number = "that many
    // elements further along".)
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    Options opt;

    // Any bad option below throws std::invalid_argument; the catch at the
    // bottom prints the message and the usage instead of crashing.
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string_view arg = args[i];

            // A lambda that returns the NEXT argument (e.g. the "5" after
            // "--depth"), or throws if there isn't one.
            // `-> std::string_view` states the lambda's return type explicitly.
            // `args[++i]` adds 1 to i FIRST, then reads that element.
            const auto next = [&]() -> std::string_view {
                if (i + 1 >= args.size()) throw std::invalid_argument("missing value after " + std::string(arg));
                return args[++i];
            };

            if (arg == "--perft") {
                const auto depth = parseInt(next());
                if (!depth || *depth < 1) throw std::invalid_argument("--perft needs a positive depth");
                // Everything after the depth is the FEN; glue the words back together.
                std::string fen;
                for (++i; i < args.size(); ++i) fen += std::string(args[i]) + ' ';
                return runPerft(*depth, fen.empty() ? std::string(Board::StartFen) : fen);
            }
            if (arg == "--bench") {
                // std::optional<int>{4} is an optional that holds 4.
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
        // std::cerr is like std::cout, but for error messages.
        std::cerr << "error: " << e.what() << "\n\n";
        printUsage();
        return EXIT_FAILURE;
    }
}
