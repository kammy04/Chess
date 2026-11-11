# Learning Log

This is a running log of the C++ concepts covered, the mistakes made (and what each one taught), and interview-ready explanations of every design choice. Each phase ends with three questions, and their answers get recorded here.

---

## Phase 0: What existed

**Finding:** the repository did not contain the engine described in the original plan. There was no minimax, no evaluation, no tests and no move generator. It held a 220-line C starter, originally written by the course instructor (June 2021) and extended by the team in February 2023, plus a README describing a Python/Pygame app that didn't exist. That code now lives, unchanged, in [`legacy/chess.c`](legacy/chess.c).

**Bugs in the C code** (worth knowing cold; each one is a C++ talking point):

| Bug | Where | Why it happens in C | What prevents it in C++ |
|---|---|---|---|
| Typing `q` writes to `board[-48][...]` | `main`: `makeMove(buf, board)` runs after the loop exits | Raw arrays have no bounds checks; `'\0' - 48 = -48` | `parseSquare` returns `std::optional`, so there's no way to get a square without checking |
| `scanf("%s", buf)` overflows on 100+ characters | `main` | No length limit | `std::getline` into a `std::string` grows as needed |
| Ctrl-D loops forever | `main` | `scanf`'s return value is ignored | `if (!std::getline(...))` exits cleanly |
| Every move is "valid" | `isValidMove`: `case 2` falls into `break`, then `return true` | Control flow nobody tested | Perft tests check every legal move against known counts |
| Queen can jump one piece | Breaks only after **2** pieces | Off-by-one | `addSliderMoves` stops at the first piece |
| Pieces have no colour | `int board[8][8]` uses only 0–6 | Magic numbers | `struct Piece { PieceType; Color; }` with `enum class` |

---

## Phase 1: The C++ design (Claude built it on request)

You asked Claude to build the baseline engine directly instead of porting nonexistent code. **Your job now is to own it.** Read each file in the order below and explain it back. The Phase 1 questions at the end check this.

Reading order: `types.hpp` → `move.hpp` → `board.hpp/.cpp` → `move_generator.cpp` → `evaluator.cpp` → `search.cpp` → `game.cpp` → `app/main.cpp` → `tests/`.

### Concepts, compared with C and Kotlin

**`enum class`** ([types.hpp](include/chess/types.hpp))
- **C:** `int` codes 0–6. A piece could be compared to a colour, or to 42, and the compiler wouldn't object.
- **C++:** `enum class Color : std::uint8_t { White, Black }` is a distinct type. Nothing converts implicitly. The `: std::uint8_t` part fixes its size at one byte.
- **Kotlin:** `enum class Color { WHITE, BLACK }`. Same idea, but Kotlin enums are objects, while C++ enums are plain integers underneath, so they cost nothing at runtime.

**Value types and aggregates** (`Piece`, `Move`)
- A `struct` with public fields and default member initialisers. `Move{from, to}` works without writing a constructor.
- `bool operator==(const Move&) const = default;` (C++20) has the compiler write the field-by-field comparison.
- **Kotlin:** like a `data class`, which generates `equals` for you.

**Classes, `private` state, `const` member functions** ([board.hpp](include/chess/board.hpp))
- `Board` keeps its arrays `private`, so only its own methods can break the invariants. One example: "`kings_` always matches where the kings are."
- `Piece pieceAt(Square) const` promises not to modify the board. That makes it callable through a `const Board&`.
- `const` correctness is checked at compile time. Kotlin's `val` only stops you reassigning a reference; C++ `const` stops you modifying the object itself.

**References vs pointers**
- `const Board&` means "borrow, read-only, never null". It's used for the evaluator and pseudo-legal generation.
- `Board&` means "borrow, may modify". It's used by `generateLegal` and `Search`, which make and unmake moves and leave the board as they found it.
- A pointer (`Board*`) can be null and can be re-pointed. We never need either, so the code has no raw pointers at all.
- **C:** you'd pass `int board[8][8]`, which decays to a pointer. You lose the size and constness unless you add them by hand.

**RAII and the "rule of zero"**
- RAII (Resource Acquisition Is Initialization) means an object acquires resources in its constructor and releases them in its destructor, automatically, when it goes out of scope.
- `std::vector` and `std::string` already do this. Since `Board` is built only from them, its compiler-generated copy, move and destructor are correct. We write **none** of them: that's the rule of zero.
- **C:** every `malloc` needs exactly one `free` on every path, including error paths. **Kotlin:** a garbage collector frees memory eventually. RAII frees it deterministically, at the closing brace.

**`std::array` vs `std::vector`**
- `std::array<Piece, 64>` has a fixed size known at compile time and lives inline, with no heap allocation. It knows its own `.size()` and doesn't decay to a pointer.
- `std::vector<Move>` is dynamic and lives on the heap. `moves.reserve(64)` pre-sizes it to avoid repeated reallocation.
- **Kotlin:** `Array` vs `MutableList`, roughly.

**`std::optional`** (`parseSquare`, `pieceFromChar`, `Game::parseMove`)
- "A value or nothing", checked by the type system. It's Kotlin's `Square?`, and `*opt` corresponds to `!!` (only safe after checking).

**`std::span`** ([move_generator.cpp](src/move_generator.cpp))
- A non-owning view (pointer + length) over contiguous data. One function accepts both an `std::array<Offset, 8>` and an `std::array<Offset, 4>`.
- **C:** you'd pass `(Offset*, size_t)` separately and hope they match.

**Header/source split and namespaces**
- `include/chess/*.hpp` declares the public API. `src/*.cpp` holds the implementation. `#pragma once` stops a header being included twice.
- [`src/offsets.hpp`](src/offsets.hpp) is a **private** header that only the library itself can see.
- An anonymous `namespace { ... }` inside a `.cpp` file is C's `static`: it hides helpers from other files.
- `namespace chess` prevents name clashes, as Kotlin packages do.

**Exceptions** (`Board::fromFen`)
- A malformed FEN throws `std::invalid_argument` with a reason. Because `fromFen` builds into a temporary `Board`, a failed parse never leaves a half-built board behind. That's the "strong exception guarantee".
- **Kotlin:** `throw IllegalArgumentException(...)`.

**Lambdas** (`isSquareAttacked`, `addPawnMoves`)
- `[&](int f, int r, PieceType t) { ... }` is a small local function that captures surrounding variables by reference. `[rank]` captures by value.

**Other C++20 features used:** structured bindings (`for (const auto& [df, dr] : offsets)`), `constexpr` (computed at compile time), `[[nodiscard]]` (warns if you ignore a return value), and digit separators (`1'000'000`).

### Design choices, in interview form

**"How do you represent the board?"**
> "Sixty-four squares in a `std::array<Piece, 64>`, with a1 = 0 and h8 = 63. Each `Piece` is a type plus a colour, both `enum class`. I also cache the king squares, because legality checking asks 'is my king attacked?' for every move. It's called a mailbox representation: easy to get right, and easy to verify with perft. Bitboards, one 64-bit integer per piece type, are faster, and they're the planned upgrade once I can measure the difference."

**"Why make/unmake instead of copying the board?"**
> "Search visits hundreds of thousands of positions. `makeMove` pushes a small `UndoInfo` (captured piece, castling rights, en-passant square, clock) onto a stack, and `unmakeMove` pops it. That's a handful of bytes per node instead of copying the whole board. The cost is correctness risk, so a test makes and unmakes every legal move in six tricky positions and checks the FEN is byte-for-byte identical."

**"How do you generate legal moves?"**
> "Two stages. First, pseudo-legal moves that follow how the pieces move. Then each one is made, I check whether my own king is attacked, and I unmake it. It's simpler than tracking pins and checks directly, and it handles rare cases such as an en-passant capture that exposes the king along a rank. Perft matches the published counts for six standard positions, which is the industry-standard proof."

**"Explain alpha-beta."**
> "Minimax explores every move. Alpha-beta keeps a window: alpha is the score I'm already guaranteed elsewhere, and beta is what my opponent is guaranteed elsewhere. If a move scores at least beta, the opponent will never allow this line, so I stop searching its siblings. That's a cutoff. The answer is provably identical to minimax, and I have a test asserting the same move and score with fewer nodes. Pruning works best when good moves are searched first, which is why move ordering is next on the roadmap."

**"Why negamax?"**
> "Chess is zero-sum, so my score is minus your score. Instead of separate max and min functions, one function maximises `-negamax(child)`. That's half the code and half the places for bugs."

**"Why `Board` vs `Game`?"**
> "`Board` knows the position. `Game` knows the history: threefold repetition, undo, whether the game is over. The search uses a bare `Board`, so it doesn't pay for bookkeeping it doesn't need. That's separation of concerns."

**"Why is `MoveGenerator` a class with only static functions?"**
> "Honestly, free functions in a namespace would be equally idiomatic in C++. It's a class because the bitboard version will hold precomputed attack tables as members, and the public interface won't change when that happens."

### Verified baseline (2026-11-11, Apple M-series, Release build)

| Check | Result |
|---|---|
| Tests | 105 pass, plus the deep perft test; also clean under AddressSanitizer + UBSan |
| Perft | All 6 reference positions match, up to 4.9M nodes (start position at depth 5: 4,865,609 in ~1.4 s) |
| Coverage (`scripts/coverage.sh`) | 99.6% of lines, 96.4% of branches |

**Baseline benchmark (`./build/chess --bench 4`).** Every Phase 3 change is compared against this:

| Position | Best move | Nodes | Time |
|---|---|---|---|
| start | b1c3 | 6,031 | 4.6 ms |
| kiwipete | e2a6 | 39,809 | 36.5 ms |
| italian | c4b5 | 86,092 | 57.7 ms |
| middlegame | c3d5 | 193,565 | 156.2 ms |
| endgame | b4f4 | 1,695 | 1.4 ms |
| **Total** | | **327,192** | **256 ms** (~1.3M nodes/s) |

Timing caveat: an earlier run took 5.3 s because a CMake build was using every CPU core at the same time. **Profile on an idle machine.** Node counts are deterministic, so they're the number to trust; time is noisy.

### Known weaknesses (honest list, and what Phase 3 is for)

- **No repetition detection inside the search.** In self-play at depth 3, White was +12 pawns ahead and drew by threefold repetition, because the search can't see the repetition coming. Zobrist hashing fixes this.
- **No quiescence search.** The evaluation stops mid-capture sequence (the "horizon effect"), so scores swing wildly between depths.
- **No move ordering.** Alpha-beta prunes far less than it could.
- **No time control.** Search runs to a fixed depth only.
- **Slow move generation.** It loops over all 64 squares and allocates a fresh `std::vector` per node.
- **No endgame king table.** The king hides in the corner even when it should be active.

---

## Mistakes log

| Phase | Mistake | Lesson |
|---|---|---|
| 0 | (C starter) Undefined behaviour from unchecked input, see the table above | Validate at the boundary; make invalid states impossible to represent |
| 1 | (Claude) Wrote the mate-in-2 test at depth 3, and it failed | Leaves return the static evaluation without generating moves, so "no legal moves = mate" is only detected one ply *after* the mating move. Mate in N moves needs depth 2N. The failing test caught a wrong assumption, which is exactly what tests are for |

*(Your mistakes go here as we work through Phases 2 onward.)*

---

## Phase questions and answers

### Phase 0
1. Why does `isValidMove` accept `03-77` for the queen?
2. What happens, line by line, when the user types `q`?
3. How would the type system have prevented the "no colours" design?

*Your answers:*

### Phase 1
1. **Explain this:** `generateLegal` takes `Board&`, but `Game::legalMoves()` is `const`. How does `legalMoves` call it without breaking its promise?
2. **What happens if** `unmakeMove` forgot to restore `enPassant_`? Which test fails first, and why that one?
3. **Explain this:** why does `Search::findBestMove` use `score > result.score` (strict), and what would change in the alpha-beta vs minimax test if it used `>=`?

*Your answers:*
