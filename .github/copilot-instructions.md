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
- Avoid runtime dynamic memory allocation.
- Allocate memory up front and divide it into subsystem-specific arenas.
- Do not add third-party libraries.
- Avoid the C standard library as much as practical.

## Testing And Build Constraints

- Keep testing inside the application.
- The executable must support a dedicated testing mode.
- Do not create a separate test harness application.
- The intended build entry point is a simple `build.ps1` script that invokes `cl.exe`.

## Working Rules

- Treat repository files as the source of truth over chat history.
- Read `AGENTS.md` first, then the relevant files in `docs\` before making substantial changes.
- Keep changes scoped to a single task or issue when possible.
- Update documentation in the same change when code moves. Use the doc ownership map in `AGENTS.md` to determine which docs to update.
- Prefer explicit data flow, deterministic behavior, and simple boundaries over clever abstractions.
