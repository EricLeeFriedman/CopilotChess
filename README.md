# CopilotChess

CopilotChess is a Windows-only chess project built to exercise harness engineering skills in an agent-first workflow.

## Start Here

1. `AGENTS.md` - entry point and document map for human and agent contributors.
2. `.github\copilot-instructions.md` - always-on repository rules for Copilot.
3. `docs\requirements.md` - product and engineering constraints.
4. `docs\architecture.md` - planned architectural boundaries for the eventual codebase.
5. `docs\testing.md` - expectations for in-application testing.
6. `docs\workflow.md` - task lifecycle for agent-driven work.
7. `docs\build-and-runner.md` - Windows self-hosted runner and build expectations.

## Current State

- Win32 application skeleton is in place (`src\main.cpp`): opens a window, runs the message pump, and supports `--test` mode.
- `build.ps1` compiles all `src\*.cpp` files to `build\chess.exe` using the local MSVC toolchain.
- CI workflow (`.github\workflows\build.yml`) runs `build.ps1` and `chess.exe --test` on every push and PR.
- GitHub issue and pull request templates are in place for structured work intake.
- Copilot cloud-agent setup is targeted at a **self-hosted Windows x64 runner**.
- Board representation (`src\board.h`, `src\board.cpp`) and memory arena subsystem (`src\memory.h`, `src\memory.cpp`) are implemented and tested.
- Pawn move generation and move application (`src\moves.h`, `src\moves.cpp`) are implemented: single push, double push, diagonal capture, en passant, and all four under-promotions (Q/R/B/N). 11 in-application tests cover these rules.

## Project Goals

- Implement a complete local two-player chess game.
- Use Win32 APIs and 2D software rendering only.
- Keep the codebase agent-legible through explicit docs, small tasks, and mechanical guardrails.
- Build testing directly into the application instead of a separate test harness executable.
