# Architecture Direction

This document defines the intended architectural shape for the codebase. It is intentionally high level and will be expanded as subsystems are implemented.

## Top-Level Principles

- Prefer simple modules with C-style function boundaries.
- Keep state explicit and visible in plain structs.
- Allocate long-lived memory at startup.
- Divide memory into distinct arenas based on lifetime and subsystem ownership.
- Avoid hidden global behavior unless the platform boundary requires it.
- Keep the design legible to future agents through small files and explicit naming.

## Planned Subsystems

### Platform

Own the Win32 application entry point, window creation, message pump, mouse input capture, timing, and file access needed by the application.

### Software Renderer

Own the pixel buffer, drawing primitives, sprite or glyph composition strategy, and presentation to the window surface using the Windows API.

### Memory

Own startup allocation and arena partitioning. A likely initial split is:

- permanent arena
- frame arena
- test arena

Additional arenas can be added only when a subsystem has a clear lifetime boundary that benefits from separation.

### Game State

Own board state, turn state, move history needed for rule evaluation, win state, and restart flow.

### Chess Rules

Own legal move generation, check detection, checkmate detection, and any supporting rule validation.

### User Interface

Own board presentation, drag-and-drop interaction state, move feedback, and visible status messages such as check or checkmate.

### In-Application Tests

Own deterministic validation of subsystem behavior inside the application executable. Tests should be able to exercise chess rules and other low-level logic without requiring human interaction.

## Dependency Direction

The repository should bias toward the following dependency shape:

`platform -> renderer -> ui -> game_state -> chess_rules`

Support systems such as memory and test infrastructure may be referenced where needed, but dependencies should remain one-directional and easy to explain.

## Architectural Non-Goals

- No class hierarchies for pieces or screens.
- No runtime plugin system.
- No dependency on external rendering, windowing, or test frameworks.
