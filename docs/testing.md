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

The `RunTests()` function in `src\main.cpp` is the entry point for all tests. Add test calls there as subsystems are built.

## Execution Expectations

- Tests run without opening an interactive window when `--test` is passed.
- New gameplay behavior should add or update tests in the same change whenever practical.

## Why This Matters

The project goal is to practice harness engineering. That means the application must expose enough observable behavior for an agent to verify changes without relying on a separate test executable or manual-only checking.
