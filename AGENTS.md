# AGENTS.md

This repository is organized for an **agent-first workflow**.

Humans set direction, define constraints, and validate results. Agents are expected to do the design, implementation, review preparation, and pull request work inside the repository.

## Entry Points

Read these files in order before making changes:

1. `README.md`
2. `docs\requirements.md`
3. `docs\architecture.md`
4. `docs\testing.md`
5. `docs\workflow.md`
6. `docs\build-and-runner.md`

## Repository Rules

- Treat repository-local documents as the source of truth.
- Keep changes small and scoped to a single task or issue.
- Update docs in the same change when requirements, architecture, or workflow assumptions change.
- Do not add third-party libraries.
- Do not introduce object-oriented boundaries. Prefer C-style module APIs and plain structs.
- Avoid dynamic allocation during runtime. Favor startup allocation and dedicated arenas.
- Preserve the Windows-only, Win32-based direction of the project.
- Keep testing inside the application as a first-class mode of operation.

## Product Constraints

- The product is a chess game.
- All pieces and legal moves must follow standard chess rules.
- The game must surface check and checkmate to the user.
- The game must be restartable after a win.
- Input is local mouse-only click-and-drag.
- Rendering is 2D software rendering through the Windows API.

## Expected Task Flow

1. Start from a GitHub issue or equivalent written task definition.
2. Confirm the relevant docs cover the task. Expand docs first when needed.
3. Implement the smallest complete change that satisfies the task.
4. Run the appropriate validation path, including the application's built-in test mode once it exists.
5. Self-review against the issue, docs, and pull request template.
6. Submit a pull request with clear validation notes and any follow-up tasks.

## Design Biases

- Optimize for correctness, legibility, and deterministic behavior over cleverness.
- Prefer explicit data flow and simple module boundaries over abstraction-heavy designs.
- Encode important invariants in docs, scripts, and checks so future agents can discover them mechanically.

## When Docs and Code Disagree

Fix the disagreement in the same change if possible. If the implementation is correct and the docs are stale, update the docs. If the docs are correct and the implementation is wrong, fix the implementation and keep the docs aligned.
