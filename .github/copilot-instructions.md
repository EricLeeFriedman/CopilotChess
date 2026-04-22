# Copilot Instructions

These rules are always-on repository guidance for Copilot.

## Core Product Constraints

- Build a local two-player chess game.
- All pieces and moves must follow standard chess rules.
- Surface check and checkmate clearly in the application.
- Allow the game to restart after a winner is decided.

## Platform And UI Constraints

- Windows only.
- Use Win32 APIs.
- Rendering must be 2D software rendering through the Windows API.
- Input is mouse-only local play on one machine.
- Piece movement should be implemented with click-and-drag interaction.

## Implementation Constraints

- Use C++.
- Do not use object-oriented design as the primary structure.
- Prefer C-style module APIs and plain structs with public data.
- Use the project's sized integer types (`int8`, `uint8`, `int32`, `uint64`, etc. from `src/types.h`) instead of built-in or CRT integer types. `uint64` is used in place of `size_t` for all sizes and offsets. `bool` and `void*` are fine as-is.
- Avoid runtime dynamic memory allocation.
- Allocate memory up front and divide it into subsystem-specific arenas.
- Do not add third-party libraries.
- Avoid the C standard library as much as practical. Win32 headers such as `windows.h` are required by the platform and are always allowed; this rule targets C standard library headers like `<stdio.h>`, `<stdlib.h>`, and `<string.h>`.
- Static `const` data at file scope is fine and is preferred over forcing compile-time constants or lookup tables into memory arenas.

## Testing And Build Constraints

- Keep testing inside the application.
- The executable must support a dedicated testing mode.
- Do not create a separate test harness application.
- Each subsystem's tests live in a dedicated `<subsystem>_tests.cpp` file. `RunTests()` in `main.cpp` calls each subsystem's `RunXxxTests` entry point.
- The intended build entry point is a simple `build.ps1` script that invokes `cl.exe` on all `*.cpp` files in `src\`.

## Working Rules

- Treat repository files as the source of truth over chat history.
- Read `AGENTS.md` first, then the relevant files in `docs\` before making substantial changes.
- Keep changes scoped to a single task or issue when possible.
- Update documentation in the same change when code moves. Use the doc ownership map in `AGENTS.md` to determine which docs to update.
- Prefer explicit data flow, deterministic behavior, and simple boundaries over clever abstractions.
