# Learning C++ with a Chess Engine

This guide assumes you have never seen C++ before. It has four parts:

1. **[How to study this project](#1-how-to-study-this-project)**: what to read, in what order.
2. **[C++ from zero](#2-c-from-zero)**: every language feature used in the code, explained simply, with a pointer to where you can see it.
3. **[Chess-engine words](#3-chess-engine-words)**: a glossary of the chess and engine terms used here.
4. **[How the engine works, and why](#4-how-the-engine-works-and-why)**: design decisions, written the way you'd explain them in an interview.

At the end are [the measured baseline](#5-the-measured-baseline), the [known weaknesses and roadmap](#6-known-weaknesses-and-roadmap), the [mistakes log](#7-mistakes-log) and the [check-your-understanding questions](#8-check-your-understanding).

---

## 1. How to study this project

**The code is the textbook.** Every file has comments explaining each new C++ idea the first time it appears. Later files say "see types.hpp" instead of repeating, so **read in this order**:

| # | File | What it is | Main C++ ideas introduced |
|---|---|---|---|
| 1 | [include/chess/types.hpp](include/chess/types.hpp) | Colours, pieces, squares | comments, `#include`, namespaces, `enum class`, functions, `struct`, `const`, type aliases, `std::optional` |
| 2 | [src/types.cpp](src/types.cpp) | Their code | header vs source, `if`, variables, `static_cast`, `auto`, `std::array` |
| 3 | [include/chess/move.hpp](include/chess/move.hpp) + [src/move.cpp](src/move.cpp) | What a move is | aggregates, printing with `<<`, defining member functions outside the struct |
| 4 | [src/offsets.hpp](src/offsets.hpp) | Piece directions | private headers, nested namespaces |
| 5 | [include/chess/board.hpp](include/chess/board.hpp) | The position | **classes, public/private, constructors, references, `std::vector`, RAII** |
| 6 | [src/board.cpp](src/board.cpp) | Board's code | `switch`, exceptions, loops, lambdas, structured bindings, `assert`, bit operators |
| 7 | [include/chess/move_generator.hpp](include/chess/move_generator.hpp) + [.cpp](src/move_generator.cpp) | Listing legal moves | static member functions, `std::span`, recursion |
| 8 | [include/chess/evaluator.hpp](include/chess/evaluator.hpp) + [.cpp](src/evaluator.cpp) | Scoring a position | returning a const reference |
| 9 | [include/chess/search.hpp](include/chess/search.hpp) + [.cpp](src/search.cpp) | Choosing a move | member initializer lists, `explicit`, `std::chrono` |
| 10 | [include/chess/game.hpp](include/chess/game.hpp) + [.cpp](src/game.cpp) | A whole game | function overloading, delegating constructors, iterators and algorithms |
| 11 | [app/main.cpp](app/main.cpp) | The program you run | `main`, keyboard input, `try`/`catch` |
| 12 | [tests/types_test.cpp](tests/types_test.cpp), then the other tests | Checks that it all works | GoogleTest |
| 13 | [CMakeLists.txt](CMakeLists.txt) | Build instructions | CMake (not C++) |

**How to read a file:** don't try to memorise it. For each block, ask yourself "what does this do, and why is it written this way?" If a comment doesn't answer that, ask me.

**Run things as you go.** Seeing the code work makes it stick:
```sh
cmake -S . -B build          # one-time setup
cmake --build build -j       # compile (re-run after any change)
./build/chess                # play
./build/chess_tests          # run every test
```

**Experiment.** Change something small, such as a piece value in `evaluator.hpp`, rebuild, and see what happens. Then undo it with `git checkout -- <file>`.

---

## 2. C++ from zero

### 2.1 What a C++ program is, and how it gets built

You write **source code** as text files. A program called the **compiler** (on your Mac, `clang++`) translates that text into **machine code**, the raw instructions your CPU runs. Unlike Python, C++ is translated *before* it runs, not while it runs. That's a large part of why it's fast, and also why mistakes in the code ("compile errors") are reported before the program even starts.

Building happens in two stages:
1. **Compiling.** Each `.cpp` file is translated on its own into an "object file".
2. **Linking.** The **linker** joins the object files, plus any libraries, into one runnable program.

With many files, you don't run those commands by hand. **CMake** reads [CMakeLists.txt](CMakeLists.txt) and runs them for you. This project builds three things:
- `chess_core`: the engine, as a **library** (reusable code that isn't a program on its own)
- `chess`: the playable program, from `app/main.cpp`
- `chess_tests`: the test program

### 2.2 Header files (`.hpp`) and source files (`.cpp`)

- A **header** *declares* what exists: "there's a function `parseSquare` that takes text and returns maybe-a-square." It's the menu.
- A **source file** *defines* how it works: the instructions inside the function. It's the kitchen.
- `#include "chess/types.hpp"` copies a header's text in, so the current file knows those names exist.
- `#pragma once` at the top of a header means "only include me once per compile", which prevents duplicate definitions.
- `#include <vector>` (angle brackets) means a standard-library header; `#include "..."` (quotes) means one of our own files.

Why split them? Other files only need to know *how to call* something, not how it works inside. Change the inside of a `.cpp` file and only that file needs recompiling. See: the top of [types.hpp](include/chess/types.hpp) and [types.cpp](src/types.cpp).

### 2.3 Comments, statements and blocks

- `// comment to end of line` and `/* comment block */`. The compiler ignores them.
- A **statement** is one instruction, and it ends with `;`.
- A **block** `{ ... }` groups statements. Variables created inside a block disappear at its closing `}`. This is called **scope**.

### 2.4 Variables and types

A **variable** is a named box that holds a value. In C++ every variable has a fixed **type**: what kind of value it holds.

```cpp
int count = 0;          // whole number (can be negative)
bool done = false;      // true or false
char letter = 'e';      // ONE character: single quotes
double half = 0.5;      // number with a decimal point
std::string name = "Kiwipete";  // text: double quotes
```

| Type | Holds | Seen in |
|---|---|---|
| `int` | whole numbers, e.g. -3, 0, 42 | everywhere |
| `bool` | `true` / `false` | everywhere |
| `char` | one character, e.g. `'a'` | [types.cpp](src/types.cpp) |
| `double` | decimals, e.g. 1.5 | [main.cpp](app/main.cpp) `formatScore` |
| `std::uint8_t` | 0 to 255, exactly 1 byte | `Color`, castling rights |
| `std::uint64_t` | 0 to about 18 quintillion | perft counts, node counts |
| `std::size_t` | sizes and positions in lists (never negative) | indexing arrays |
| `std::string` | text that can change | FEN, messages |
| `std::string_view` | read-only view of existing text (no copy) | function parameters |

**Literals** are values typed directly into code: `42`, `'a'` (a char), `"text"` (text), `0b1111` (binary for 15), `1'000'000` (the `'` marks are just for readability), and `1U` (the U means unsigned).

**`auto`** asks the compiler to work out the type from the value: `const auto pos = letters.find(c);`. Use it when the type is obvious or very long.

**`static_cast<Type>(value)`** converts a value to another type, explicitly: `static_cast<char>('a' + 4)` gives `'e'`. C++ makes you write conversions like this so they're never accidental.

### 2.5 `const` and `constexpr`

- `const int x = 5;` means x can never change after this line. **Use `const` whenever something shouldn't change.** Then the compiler catches you if you try.
- `constexpr` means "this can be computed while compiling". The value is ready before the program even runs. It's used for tables and small helper functions.
- A `const` after a member function's `()` (e.g. `bool isEmpty() const`) promises that the function doesn't change the object. See 2.10.

### 2.6 Functions

```cpp
int add(int a, int b) {   // return type, name, parameters
    return a + b;         // hand back a value and stop
}
int five = add(2, 3);     // calling it
```
- `void` as a return type means "returns nothing".
- A **declaration** is the first line followed by `;` (no body), usually in a header. A **definition** includes the `{ body }`, usually in the `.cpp` file.
- **Default arguments:** `std::string render(bool unicode = true) const;` lets you call `render()` or `render(false)`.
- **Overloading:** two functions can share a name if their parameters differ. C++ picks the right one from what you pass. See `Game::play` in [game.hpp](include/chess/game.hpp).
- **Recursion:** a function that calls itself on a smaller problem. See `perft` in [move_generator.cpp](src/move_generator.cpp) and `negamax` in [search.cpp](src/search.cpp).
- `noexcept` promises the function never throws an exception (see 2.15).
- `[[nodiscard]]` asks the compiler to warn if someone calls the function and ignores the result.

### 2.7 Making decisions and repeating

```cpp
if (x > 0) { ... } else if (x == 0) { ... } else { ... }
int sign = x >= 0 ? 1 : -1;          // ternary: condition ? if-true : if-false

for (int i = 0; i < 8; ++i) { ... }  // classic loop: i = 0, 1, ..., 7
for (const Move& m : moves) { ... }  // range loop: once per element of moves
while (onBoard(f, r)) { ... }        // repeat while the condition is true

switch (piece.type) {                // pick a branch by value
    case PieceType::Pawn: ...; break;
    default: ...;
}
```
- `break` leaves the loop (or switch). `continue` skips to the next pass of the loop. `return` leaves the whole function.
- `++i` adds 1, `--i` subtracts 1. `x += 3` means `x = x + 3`.
- Comparisons: `==` (equal), `!=` (not equal), `<`, `>`, `<=`, `>=`. **Don't confuse `=` (store a value) with `==` (compare).**
- Logic: `&&` (and), `||` (or), `!` (not). **Short-circuit:** `a && b` doesn't even look at `b` if `a` is false. The code relies on this to avoid reading off-board squares ([board.cpp](src/board.cpp) `holds`).
- Arithmetic: `+ - * /` and `%` (remainder). `7 / 2` is `3` when both are ints: whole-number division drops the fraction.

### 2.8 Namespaces

A namespace is a named box for names, to avoid clashes. Our code is all inside `namespace chess { ... }`, so `Board` is really `chess::Board`. The standard library lives in `std`, hence `std::vector`. `::` means "the thing inside".
- `namespace { ... }` with no name (an **anonymous namespace**) means "only visible inside this .cpp file". It's used for private helpers in [board.cpp](src/board.cpp).
- `using namespace chess;` lets you drop the `chess::` prefix. That's fine inside a `.cpp` file; avoid it in headers.

### 2.9 `enum class`

A type whose value must be one of a fixed list of names:
```cpp
enum class Color : std::uint8_t { White, Black };
Color c = Color::White;
```
It's strict: you can't do arithmetic on it, or compare it to an unrelated type, by accident. When you really need it as a number, you convert explicitly with `static_cast`. See [types.hpp](include/chess/types.hpp).

### 2.10 `struct` and `class`: making your own types

A **struct** or **class** bundles data (**member variables**, or **fields**) with functions that work on that data (**member functions**):

```cpp
struct Piece {
    PieceType type = PieceType::None;   // field with a default value
    bool isEmpty() const { return type == PieceType::None; }  // member function
};
Piece p;            // create one: an OBJECT (instance) of type Piece
p.isEmpty();        // call a member function with a dot
p.type = ...;       // access a field with a dot
```

- **`public:` / `private:`**: public members can be used by anyone; private ones only by the class's own member functions. Hiding data protects the class's rules ("invariants"). **The only difference between `struct` and `class`** is the default: struct members are public, class members are private. By convention, plain data bundles are `struct`s (`Piece`, `Move`) and things with rules to protect are `class`es (`Board`, `Game`).
- **Constructor**: a function named like the class, with no return type, that runs automatically when an object is created. `Board board;` runs `Board()`. See [board.hpp](include/chess/board.hpp).
- **Member initializer list**: `Search(...) : evaluator_(e), algorithm_(a) {}` sets members as the object is being built. This is the preferred way. See [search.hpp](include/chess/search.hpp).
- **Delegating constructor**: `Game::Game() : Game(Board{}) {}` means "build me using the other constructor". See [game.cpp](src/game.cpp).
- **`explicit`** on a constructor prevents sneaky automatic conversions.
- **`static` member function**: belongs to the class, not to any object. You call it as `Board::fromFen(...)`, with no Board needed first.
- **`const` member function**: promises not to change the object, so it can be used through a `const` reference.
- **`this`**: inside a member function, the name for "the object this function was called on".
- **Defining a member outside the class**: write `ReturnType ClassName::functionName(...) { ... }` in the .cpp file.
- **Aggregate**: a simple struct with public fields and no constructor can be filled in with braces: `Move{from, to}`. Values go to the fields in the order they're declared.
- **Operator overloading**: you can define what `==` or `<<` mean for your own type. `bool operator==(const Move&) const = default;` makes the compiler write equality for you.

### 2.11 References (`&`) and pointers (`*`)

This is the most important idea that's new in C++. **By default, passing a value to a function makes a copy:**

```cpp
void f(Board b)        { ... }  // f gets its OWN COPY of the board. Changes don't affect the caller.
void f(Board& b)       { ... }  // REFERENCE: f works on the caller's ACTUAL board and can change it.
void f(const Board& b) { ... }  // CONST REFERENCE: the caller's actual board, read-only. No copy.
```

**Rules of thumb used in this project:**
- Small things such as `int`, `Square`, `Piece` and `Color`: pass by value (copying is as cheap as anything else).
- Bigger things you only need to read, such as `Board`, `Move` and `std::string`: pass `const Type&`.
- Things the function must change: pass `Type&`. Examples are `MoveGenerator::generateLegal(Board&)`, which plays and takes back moves, and `addPawnMoves(..., std::vector<Move>& moves)`, which adds to the caller's list.

A reference can never be empty. It always refers to a real object.

A **pointer** (`Type*`) is the older, lower-level tool. It holds the **memory address** of something, it *can* be empty (`nullptr`), and it can be changed to point somewhere else. `*p` follows the pointer to the thing; `&x` gives the address of `x`. **This project avoids pointers on purpose.** They appear only where an older interface demands them: `char** argv` in `main`, `const char*` text in `parseCounter` and the bench table, and `&used` passed to `std::stoi`.

**Never return a reference to a local variable.** The local disappears when the function ends, leaving the reference pointing at nothing. Returning a reference is fine only for things that live longer, such as the tables in [evaluator.cpp](src/evaluator.cpp) or members of an object (`Game::board()`).

### 2.12 Copying objects (value semantics)

`Board copy = original;` makes a **complete, independent copy**: all 64 squares and the whole history list. Changing `copy` never affects `original`. `Game::legalMoves()` uses this trick: it's `const`, so it can't change `board_`, but it needs a board it can change temporarily, so it makes a copy. There's a test for this: `Board.CopiesAreIndependent`.

### 2.13 Memory: the stack, the heap, and RAII

- Ordinary local variables live on the **stack**. They're created when declared and destroyed automatically at the closing `}`. This is fast and automatic.
- When a size isn't known in advance (a list that grows), memory comes from the **heap**, a big pool the program requests from and must give back.
- **RAII** ("Resource Acquisition Is Initialization") is C++'s central idea for this: an object that requests a resource gives it back in its **destructor**, a special function that runs automatically when the object is destroyed. `std::vector` and `std::string` do this, so you never manage their memory yourself.
- Because `Board` is made only of things that already clean up after themselves, the compiler writes Board's destructor and copy operations correctly for us. Writing none of them is called the **rule of zero**.
- **This project never requests or frees memory by hand.** That's modern C++ style, and it removes a whole category of bugs (leaks, use-after-free). See the RAII note at the bottom of [board.hpp](include/chess/board.hpp).

### 2.14 Standard-library containers and helpers

| Tool | What it is | Key operations | Seen in |
|---|---|---|---|
| `std::array<T, N>` | exactly N elements, size fixed forever | `a[i]`, `.fill(x)`, `.size()` | `Board::squares_`, tables |
| `std::vector<T>` | a list that grows and shrinks | `.push_back(x)`, `.pop_back()`, `.back()`, `.size()`, `.empty()`, `.clear()`, `.reserve(n)`, `v[i]` | move lists, history |
| `std::string` | changeable text | `+` to join, `+=` to append, `.size()`, `.substr()` | FEN |
| `std::string_view` | read-only view of existing text | same reading operations, no copying | parameters |
| `std::optional<T>` | a value or nothing | `if (opt)`, `*opt`, `opt->field`, `std::nullopt` | `parseSquare` |
| `std::pair<A, B>` | two values together | `.first`, `.second` | `perftDivide` |
| `std::span<T>` | view of any contiguous list | used like an array | `addLeaperMoves` |

**The `<T>` angle brackets** fill in a **template parameter**, i.e. *which type* the container holds. `std::vector<Move>` is "a vector of Moves". You don't need to write templates yourself yet; just read `<...>` as "of ...".

**Iterators and algorithms**: `v.begin()` marks the first element, and `v.end()` marks the position just *past* the last one. Standard algorithms take such a pair:
```cpp
std::find(v.begin(), v.end(), x)   // marker to x, or v.end() if not found
std::count(v.begin(), v.end(), x)  // how many times x appears
std::max(a, b)                     // the larger value
```
See `Game::play` and `Game::result` in [game.cpp](src/game.cpp).

**Structured bindings** unpack a struct or pair into named variables: `for (const auto& [df, dr] : offsets)` and `const auto [ptr, ec] = std::from_chars(...)`.

### 2.15 Lambdas

A small function written right where it's needed, often stored in a variable:
```cpp
const auto isEven = [](int n) { return n % 2 == 0; };
isEven(4);   // true
```
The `[ ]` is the **capture list**, saying which surrounding variables the lambda may use:
- `[]`: none
- `[rank]`: a *copy* of `rank`
- `[&]`: all of them, *by reference*
- `[this]`: the current object's members

Seen in [board.cpp](src/board.cpp) (`holds`, `rayHits`, `keepIf`) and [move_generator.cpp](src/move_generator.cpp).

### 2.16 Errors: exceptions, `assert`, and undefined behaviour

- **Exceptions** are for errors caused by bad *input*. `throw std::invalid_argument("message");` stops the current function and every caller, until a `try { ... } catch (const std::invalid_argument& e) { ... }` handles it. `e.what()` gives the message. In this project, `Board::fromFen` throws on bad FEN, and `main.cpp` catches it.
- **`assert(condition)`** is for *programmer* mistakes. In Debug builds the program stops if the condition is false; in Release builds the check is removed. See `makeMove`.
- **Undefined behaviour** is what happens when a program breaks C++'s rules, for example reading `squares_[70]` in a 64-element array. C++ doesn't check by default (that's part of why it's fast), and the result is unpredictable: it might crash, give wrong answers, or seem to work. Defences: careful code, tests, and **sanitizers** (a special build that checks every memory access). This project passes all tests under AddressSanitizer and UndefinedBehaviorSanitizer:
  ```sh
  cmake -S . -B build-asan -DCHESS_ENABLE_SANITIZERS=ON && cmake --build build-asan -j && ./build-asan/chess_tests
  ```

### 2.17 Bits and bitwise operators

A number is stored in binary. One byte (`std::uint8_t`) is 8 binary digits ("bits"), for example `0000 1011`. We use the four lowest bits as four on/off switches for the castling rights:

| Operator | Meaning | Use |
|---|---|---|
| `a \| b` | bit on if on in a **or** b | combine flags |
| `a & b` | bit on if on in **both** | test or keep flags |
| `~a` | flip every bit | "everything except" |
| `1U << n` | 1 moved left n places | the flag for bit n |
| `x \|= f` | turn flag f on | |
| `x &= ~f` | turn flag f off | |
| `(x & f) != 0` | is flag f on? | |

See the `castling` namespace in [board.hpp](include/chess/board.hpp) and `castlingMaskFor` in [board.cpp](src/board.cpp). Bitboards (a later roadmap step) take this idea much further.

### 2.18 Input and output

- `std::cout << "text" << value << '\n';` prints to the terminal. `'\n'` is a newline.
- `std::cerr` is the same, but for error messages.
- `std::getline(std::cin, line)` reads a whole typed line. It's false when input ends (Ctrl-D).
- `std::istringstream` reads *from* a string as if it were typed input (`>>` takes the next word). `std::ostringstream` prints *into* a string (`.str()` gets the result).
- `std::setw`, `std::setprecision`, `std::fixed` and `std::showpos` control number formatting.

### 2.19 `main` and command-line arguments

`int main(int argc, char** argv)` is where every C++ program starts. `argc` is how many words were typed (including the program name), and `argv` holds those words. The return value is the exit code: 0 for success. See [main.cpp](app/main.cpp).

### 2.20 Testing with GoogleTest

Explained at the top of [tests/types_test.cpp](tests/types_test.cpp). In short: `TEST(Group, Name) { EXPECT_EQ(actual, expected); }`. `ctest --test-dir build` runs everything, and `./scripts/coverage.sh` shows which lines the tests exercise.

---

## 3. Chess-engine words

| Term | Meaning |
|---|---|
| **Square numbering** | a1 = 0, b1 = 1, ..., h1 = 7, a2 = 8, ..., h8 = 63 (diagram in [types.hpp](include/chess/types.hpp)) |
| **File / rank** | a column (a to h) / a row (1 to 8). In code both are 0 to 7 |
| **FEN** | one line of text describing a position, e.g. `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`. The fields are: pieces (rank 8 first, a digit = that many empty squares, uppercase = White), side to move, castling rights, en-passant square, half-move clock, move number |
| **Castling** | king moves two squares toward a rook, and the rook jumps to the king's other side. Not allowed if the king or that rook has moved, if pieces are in between, or if the king is in check, passes through check, or lands in check |
| **En passant** | right after a pawn moves two squares, an enemy pawn beside it may capture it as if it had moved one. The en-passant square is the square it skipped |
| **Promotion** | a pawn reaching the last rank becomes a queen, rook, bishop or knight |
| **Check / checkmate / stalemate** | king attacked / attacked with no legal escape (loss) / not attacked but no legal moves (draw) |
| **Fifty-move rule** | draw after 50 moves by each side (100 half-moves) with no capture or pawn move |
| **Threefold repetition** | draw when the same position occurs three times |
| **Insufficient material** | draw when neither side has enough pieces left to checkmate |
| **Pseudo-legal / legal move** | follows piece movement / also doesn't leave your own king in check |
| **Ply** | one move by one player (a "half-move"). Depth is measured in plies |
| **Node** | one position visited during search |
| **Perft** | counts every possible move sequence to a fixed depth. Correct counts are published, so a match proves the move generator right |
| **Centipawn** | 1/100 of a pawn: the unit of evaluation scores |
| **Piece-square table** | a bonus per square per piece type (e.g. knights love the centre) |
| **Minimax** | look ahead assuming both sides always choose their best move |
| **Negamax** | minimax written with one function, using "my score = −(opponent's score)" |
| **Alpha-beta** | minimax that skips lines which can't change the result. Same answer, far fewer nodes |
| **Horizon effect** | the search stops at a fixed depth, so it can't see a capture one move past its limit |
| **UCI** | the text protocol chess GUIs use to talk to engines. Moves are written like `e2e4` |

---

## 4. How the engine works, and why

The flow of one engine move:

```
main.cpp ──► Search::findBestMove(board, depth)
               │  for each legal move (MoveGenerator::generateLegal)
               │      board.makeMove(move)
               │      score = -negamax(...)     ◄── recursion, depth-1 each level
               │      board.unmakeMove()
               │  at depth 0: Evaluator::evaluate(board)
               ▼
           best move + score + node count + time
```

Each design choice below is written as you could explain it in an interview.

**"How do you represent the board?"**
> "As 64 squares in a fixed-size array, `std::array<Piece, 64>`, numbered a1 = 0 to h8 = 63. Each `Piece` is a type plus a colour, both strongly typed enums, so the compiler stops me mixing them up. I also store where the two kings are, because legality checking constantly asks 'is my king attacked?'. This layout is called a mailbox. It's simple to get right and easy to verify. Bitboards, one 64-bit number per piece type, are faster, and they're on my roadmap once I can measure the gain."

**"Why play moves and take them back, instead of copying the board?"**
> "The search visits hundreds of thousands of positions per move. `makeMove` saves a small record (the captured piece, castling rights, en-passant square, clock) on a stack, and `unmakeMove` restores it. That's a few bytes per position instead of copying the whole board. The risk is a bug where undo forgets something, so there's a test that makes and unmakes every legal move in six tricky positions and checks the position text is identical afterwards."

**"How do you know your move generator is correct?"**
> "Perft. It counts every move sequence to a given depth, and the chess-programming community publishes the exact counts for standard test positions. My generator matches all six positions I test, up to about 4.9 million positions deep. One wrong rule anywhere would change the numbers."

**"How do you generate legal moves?"**
> "In two steps. First I generate moves by how pieces move. Then I try each one, check whether my own king is attacked, and take it back. It's simpler than computing pins and checks directly, and it automatically handles rare cases like an en-passant capture that exposes the king along a row."

**"Explain alpha-beta pruning."**
> "Minimax explores every line. Alpha-beta keeps two bounds: alpha is the best score I'm already guaranteed through another move, and beta is the best my opponent is guaranteed elsewhere. If I find a reply that scores at least beta, the opponent would never enter this line, so I can skip my remaining moves here. That's a cutoff. The answer is provably identical to minimax, and I have a test that runs both on six positions and asserts the same move and score with fewer nodes. Pruning works best when the best moves are tried first, which is why move ordering is the next improvement."

**"Why negamax?"**
> "Chess is zero-sum, so my score is minus my opponent's. Instead of separate 'maximise' and 'minimise' functions, one function maximises the negated child scores. Half the code, half the places for bugs."

**"Why separate `Board` and `Game`?"**
> "Separation of concerns. `Board` knows the current position. `Game` knows the history needed for repetition draws, undo and game-over detection. The search uses a bare `Board`, so it doesn't pay for bookkeeping it doesn't need."

**"Why are there no `new`/`delete` or raw pointers?"**
> "RAII. All memory is owned by standard containers whose destructors free it, so the compiler-generated copy and destruction are correct. That's the rule of zero. It removes leaks and dangling pointers as a category of bug, and I verified it with AddressSanitizer."

**"Why is `MoveGenerator` a class when all its functions are static?"**
> "Free functions in a namespace would work just as well today. It's a class so a faster bitboard version can keep precomputed attack tables as members without changing how anyone calls it."

---

## 5. The measured baseline

Measured 2026-11-11 on an Apple-silicon Mac, Release build.

| Check | Result |
|---|---|
| Tests | 105 pass (+ 1 slow perft test you can opt into); clean under AddressSanitizer and UBSan |
| Perft | all 6 reference positions match; start position at depth 5 = 4,865,609 in ~1.4 s |
| Test coverage (`./scripts/coverage.sh`) | 99.6% of lines, 96.4% of branches |

**Benchmark (`./build/chess --bench 4`).** Every future improvement is compared against this:

| Position | Best move | Nodes | Time |
|---|---|---|---|
| start | b1c3 | 6,031 | 4.6 ms |
| kiwipete | e2a6 | 39,809 | 36.5 ms |
| italian | c4b5 | 86,092 | 57.7 ms |
| middlegame | c3d5 | 193,565 | 156.2 ms |
| endgame | b4f4 | 1,695 | 1.4 ms |
| **Total** | | **327,192** | **256 ms** (~1.3 million nodes/s) |

**Measure on an idle machine.** A run during a heavy background build took 5.3 s instead of 256 ms. Node counts never change between runs, so trust nodes more than time.

---

## 6. Known weaknesses and roadmap

Honest weaknesses of the current engine:
- **It can't see repetitions while searching.** In an engine-vs-engine game, White was 12 pawns ahead and still allowed a draw by threefold repetition.
- **No quiescence search.** The search can stop in the middle of a capture sequence (the horizon effect), so scores jump around between depths.
- **No move ordering.** Alpha-beta prunes far less than it could.
- **No time limit.** It searches to a fixed depth however long that takes.
- **Simple move generation.** It loops over all 64 squares and creates a new list at every position.
- **No endgame king table.** The king hides in the corner even when it should come out.

Roadmap. **You** write each step and I review it. Every step is measured before and after with `--bench`:
1. Move ordering (captures first, MVV-LVA)
2. Iterative deepening with a time limit
3. Quiescence search
4. Zobrist hashing and a transposition table (this also fixes repetition detection)
5. Bitboards
6. UCI protocol, so the engine runs in real chess GUIs
7. Multithreaded search

---

## 7. Mistakes log

Mistakes and what each one taught. Yours will be added here as we go.

| Who | Mistake | Lesson |
|---|---|---|
| Claude | Wrote the "mate in 2" test at depth 3, and it failed | The search only notices "no legal moves = checkmate" at a position it *expands* (depth > 0). Positions at depth 0 just get a score. So a mate delivered on ply 3 is detected at depth 4. Mate in N moves needs depth 2N. The failing test exposed a wrong assumption, which is exactly what tests are for |

---

## 8. Check your understanding

Answer in your own words, out loud or in writing. Answers get recorded here.

### Part 1: types.hpp, types.cpp, move.hpp, move.cpp
1. **Explain this:** what's the difference between `'e'` and `"e"` in C++, and why does `squareToString` use `static_cast<char>`?
2. **What happens if** you call `parseSquare("z9")`? What does it return, and how should the caller check it?
3. **Explain this:** in `Piece`, what does the word `const` mean in `bool isEmpty() const`, and why does it matter?

*Your answers:*

### Part 2: board.hpp, board.cpp
1. **Explain this:** what's the difference between `void f(Board b)`, `void f(Board& b)` and `void f(const Board& b)`? Which one would you use to print a board, and why?
2. **What happens if** `unmakeMove` forgot to restore `enPassant_`? Which test would fail?
3. **Explain this:** `history_` is a `std::vector` that requests memory as it grows. Who gives that memory back, and when?

*Your answers:*

### Part 3: move generator, evaluator, search, game
1. **Explain this:** `MoveGenerator::generateLegal` takes `Board&`, but `Game::legalMoves()` is `const`. How does `legalMoves` call it without breaking its promise?
2. **What happens if** `Search::findBestMove` used `>=` instead of `>` when comparing scores?
3. **Explain this, interview-style:** how does alpha-beta give the same answer as minimax while looking at fewer positions?

*Your answers:*
