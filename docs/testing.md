# Testing Strategy

Testing is part of the product, not an external harness.

## Required Capabilities

- The application executable must support a dedicated testing mode.
- Test results must be deterministic and suitable for automation.
- Failures must be surfaced clearly enough for an agent to diagnose them from logs or process output.

## Initial Test Priorities

1. Chess rule validation
2. Check and checkmate detection
3. Restart and game-state reset behavior
4. Input-to-move translation for drag-and-drop interactions
5. Memory and arena invariants that can be validated without user input

## How To Run Tests

```
.\build\chess.exe --test
```

Exits with code `0` when all tests pass, `1` on any failure. The CI workflow (`build.yml`) runs this automatically on every push and PR.

## Test File Convention

Each subsystem gets a dedicated test file named `<subsystem>_tests.cpp` (e.g., `memory_tests.cpp`). That file:

- Includes the subsystem header and `tests.h`
- Defines all test functions for that subsystem as `static` (internal to the file)
- Uses a file-static pointer to receive shared test state (e.g., `AppMemory*`) set by the suite entry point
- Declares a file-static `TestEntry` array using the `TEST_ENTRY(fn)` macro — one entry per test function
- Exports a single `RunXxxTests(AppMemory*, int32* passed, int32* total)` function that sets state, calls `RunTestArray`, and accumulates counts into the output parameters

`src/main.cpp` forward-declares each `RunXxxTests` function and calls them from `RunTests()`. `src/tests.h` provides the `TestEntry` struct, `TEST_ENTRY` macro, and `RunTestArray` helper.

### Adding a new test

1. Write a `static bool MyTest_Foo(void)` function in the appropriate `*_tests.cpp` file.
2. Append `TEST_ENTRY(MyTest_Foo)` to that file's `k_XxxTests[]` array.

No other changes are required — `RunTestArray` iterates the array automatically.

## Test Infrastructure Files

| File | Purpose |
|---|---|
| `src/tests.h` | `TestEntry` struct, `TEST_ENTRY` macro, and `RunTestArray` helper shared by all test files |
| `src/memory_tests.cpp` | Tests for the memory and arena subsystem |
| `src/board_tests.cpp` | Tests for board initialisation |
| `src/moves_tests.cpp` | Tests for pawn move generation, knight move generation, rook/bishop/queen (sliding piece) move generation, king move generation, move application (including en passant, promotion, and castling), check detection (`IsInCheck`) including pawn-forward-push-is-not-check, legal move filtering (`GetLegalMoves`) including pin scenarios, the pinned-enemy-attacker regression (a pinned enemy knight still blocks a legal king step per FIDE Article 3.8), and the king-capture exclusion regression (legal moves never include capturing the opponent's king), castling generation and rights tracking (`GenerateCastlingMoves`), the discovered-attack regression for castling (an enemy rook masked by the king on its starting square still prohibits castling through the transit squares it would control once the king moves), and position evaluation (`EvaluatePosition`) including Fool's mate checkmate detection, stalemate detection, and the ongoing-position base case |
| `src/renderer_tests.cpp` | Tests for the software renderer: `ClearBuffer` pixel fill, `DrawRect` basic fill, clipping on all four edges, fully out-of-bounds rects, and zero-size rects; `DrawFilledCircle` center pixel set, zero radius (no pixels drawn), and clips-to-buffer (no crash) |
| `src/ui_tests.cpp` | Tests for the UI subsystem: `DrawBoard` does not crash on a freshly initialized `GameState`; the center of a selected empty square is exactly `BOARD_SELECTED` (positive equality assertion); the center of an empty legal-move target is exactly `BOARD_MOVE_DOT` (positive equality assertion), and a non-target square retains its plain board color |
| `src/input_tests.cpp` | Tests for the input subsystem: `PixelToSquare` corner/center/boundary/out-of-bounds mapping; `InputInit` and `InputCancelDrag` state reset; `InputHandleDragStart` (own piece, opponent piece, empty square, outside board); `InputHandleDragMove` cursor update and no-op-when-not-dragging; `InputHandleDragEnd` legal move application, illegal target cancellation, out-of-board cancellation, no-op-when-not-dragging; `InputCancelDrag` clears an active drag; `InputHandleDragEnd` enters `pending_promotion` on pawn drop at the last rank; `InputHandlePromotionClick` applies queen and knight underpromotion, cancels on wrong file, cancels outside rank range, cancels outside board, and is a no-op when not in pending promotion state |
| `src/main.cpp` `RunTests()` | Top-level runner — calls each `RunXxxTests`, accumulates counts, and prints the summary line |

## Execution Expectations

- Tests run without opening an interactive window when `--test` is passed.
- New gameplay behavior should add or update tests in the same change whenever practical.

## Why This Matters

The project goal is to practice harness engineering. That means the application must expose enough observable behavior for an agent to verify changes without relying on a separate test executable or manual-only checking.
