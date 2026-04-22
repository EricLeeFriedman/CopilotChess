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

## Execution Expectations

- Tests should run without opening an interactive gameplay session when test mode is selected.
- The default build and validation flow should eventually be scriptable through PowerShell for both humans and agents.
- New gameplay behavior should add or update tests in the same change whenever practical.

## Why This Matters

The project goal is to practice harness engineering. That means the application must expose enough observable behavior for an agent to verify changes without relying on a separate test executable or manual-only checking.
