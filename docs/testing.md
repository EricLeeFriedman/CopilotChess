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
- Exports a single `RunXxxTests(...)` function that sets state, runs all tests via `RUN_TEST`, and returns `bool`

`src/main.cpp` forward-declares each `RunXxxTests` function and calls them from `RunTests()`. `src/tests.h` provides the shared `RUN_TEST` macro.

## Test Infrastructure Files

| File | Purpose |
|---|---|
| `src/tests.h` | `RUN_TEST` macro shared by all test files |
| `src/memory_tests.cpp` | Tests for the memory and arena subsystem |
| `src/board_tests.cpp` | Tests for board initialisation |
| `src/moves_tests.cpp` | Tests for pawn move generation, knight move generation, rook/bishop/queen (sliding piece) move generation, move application, and check detection (`IsInCheck`) |
| `src/main.cpp` `RunTests()` | Top-level runner — calls each `RunXxxTests` |

## Execution Expectations

- Tests run without opening an interactive window when `--test` is passed.
- New gameplay behavior should add or update tests in the same change whenever practical.

## Why This Matters

The project goal is to practice harness engineering. That means the application must expose enough observable behavior for an agent to verify changes without relying on a separate test executable or manual-only checking.
