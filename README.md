# Terminal Chess Engine (C++20)

A chess engine and terminal chess game written in modern C++. You can play against a friend or against the engine, or watch the engine play itself.

It's also a learning project: every line of the code is commented for someone new to C++, and [LEARNING.md](LEARNING.md) is a step-by-step guide to the language using this codebase.

## Features

**Rules (complete)**
- Every piece moves correctly, including castling, en passant and promotion (choose any piece; queen by default)
- Legal-move checking: you can't leave your own king in check, castle through check, or play an en-passant capture that exposes your king
- Game endings: checkmate, stalemate, threefold repetition, the fifty-move rule and insufficient material
- Move history, undo and reset, and loading any position from FEN

**Engine**
- Negamax search with alpha-beta pruning (plain minimax is kept as a reference implementation)
- Evaluation from material plus piece-square tables
- Mate-distance scoring: it prefers faster mates and slower losses

**Verification**
- 100+ GoogleTest tests, including perft on six standard positions. Perft counts the full move tree and checks it against published numbers (16 million+ positions match at deeper depths)
- A test that alpha-beta returns exactly the same move and score as minimax while searching fewer nodes
- A clean run under AddressSanitizer and UndefinedBehaviorSanitizer
- A coverage report via `scripts/coverage.sh`

## Build

Requires CMake ≥ 3.20 and a C++20 compiler (Apple clang 15+, GCC 11+, or recent MSVC). GoogleTest is downloaded automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Play

```sh
./build/chess                         # you (White) vs the engine, depth 4
./build/chess --color black --depth 5
./build/chess --mode pvp              # two humans
./build/chess --mode eve              # engine vs engine
./build/chess --ascii                 # letters instead of chess glyphs
```

Enter moves in coordinate notation, such as `e2e4`, or `e7e8n` to promote to a knight. Other commands: `moves`, `undo`, `reset`, `history`, `fen`, `load <FEN>`, `help`, `quit`.

## Measure

```sh
./build/chess --perft 5                       # move-generator check (start position: 4,865,609)
./build/chess --perft 3 "<FEN>"               # any position, split by root move
./build/chess --bench 4                       # nodes / time / best move on fixed positions
./scripts/coverage.sh --html                  # test coverage report
cmake -S . -B build-asan -DCHESS_ENABLE_SANITIZERS=ON   # memory-safety build
```

## Layout

```
include/chess/   public headers: types, move, board, move_generator, evaluator, search, game
src/             implementations (+ private header offsets.hpp)
app/main.cpp     terminal UI, perft and bench tools
tests/           GoogleTest suite
LEARNING.md      C++ from zero, chess-engine glossary, design explanations
```

## Roadmap

Each step is measured before and after with `--bench`:

1. Move ordering (MVV-LVA)
2. Iterative deepening with a time limit
3. Quiescence search
4. Zobrist hashing and a transposition table (this also gives repetition detection inside the search)
5. Bitboards
6. The UCI protocol, so the engine can run inside chess GUIs
7. Multithreaded search

## Contact

- **Email**: [ukaegbuc@mcmaster.ca](mailto:ukaegbuc@mcmaster.ca)
- **GitHub**: [kammy04](https://github.com/kammy04)
