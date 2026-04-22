# CopilotChess

CopilotChess is a Windows-only chess project built to exercise harness engineering skills in an agent-first workflow.

This repository intentionally starts with **no application code**. The first commit establishes the constraints, documentation, and GitHub scaffolding that future agents will use for design, implementation, review, and submission.

## Start Here

1. `AGENTS.md` - entry point and document map for human and agent contributors.
2. `.github\copilot-instructions.md` - always-on repository rules for Copilot.
3. `docs\requirements.md` - product and engineering constraints.
4. `docs\architecture.md` - planned architectural boundaries for the eventual codebase.
5. `docs\testing.md` - expectations for in-application testing.
6. `docs\workflow.md` - task lifecycle for agent-driven work.
7. `docs\build-and-runner.md` - Windows self-hosted runner and build expectations.

## Current State

- No game code has been added yet.
- GitHub issue and pull request templates are in place for structured work intake.
- Copilot cloud-agent setup is targeted at a **self-hosted Windows x64 runner**.

## Project Goals

- Implement a complete local two-player chess game.
- Use Win32 APIs and 2D software rendering only.
- Keep the codebase agent-legible through explicit docs, small tasks, and mechanical guardrails.
- Build testing directly into the application instead of a separate test harness executable.
