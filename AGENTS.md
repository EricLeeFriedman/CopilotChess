# AGENTS.md

This repository is organized for an **agent-first workflow**.

Start here, then follow the linked documents. The always-on repository rules live in `.github\copilot-instructions.md`.

## Entry Points

Read these files in order before making changes:

1. `README.md`
2. `.github\copilot-instructions.md`
3. `docs\requirements.md`
4. `docs\architecture.md`
5. `docs\testing.md`
6. `docs\workflow.md`
7. `docs\build-and-runner.md`

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

## Document Roles

- `AGENTS.md` is the map.
- `.github\copilot-instructions.md` holds the always-on repository rules.
- `docs\` holds deeper design, workflow, testing, and platform guidance.

## Doc Ownership Map

When you make a change, update the docs in the right column of this table in the same commit.

| Change type | Update these docs |
|---|---|
| New subsystem or significant structural change to `src\` | `docs\architecture.md` |
| New test or change to test infrastructure, `RunTests()`, or any `*_tests.cpp` file | `docs\testing.md` |
| Change to `build.ps1` or compiler flags | `docs\build-and-runner.md` |
| Change to any file in `.github\workflows\` | `docs\build-and-runner.md` |
| Change to a gameplay rule, input behavior, or win condition | `docs\requirements.md` |
| Change to the task or PR workflow | `docs\workflow.md` |
| Change to review routing logic or PR classification in `pr-review.yml` | `docs\workflow.md`, `docs\build-and-runner.md` |
| Change to agent prompt scope or behavior (`.github\agents\**`) | `docs\workflow.md` |
| Major milestone: new subsystem working end-to-end | `README.md` Current State |
| Change to a core constraint (platform, memory, testing, build) | `.github\copilot-instructions.md` |

If a change touches multiple rows, update all the listed docs. If a doc update would be a lie (nothing actually changed in that area), skip it.

## When Docs and Code Disagree

Fix the disagreement in the same change if possible. If the implementation is correct and the docs are stale, update the docs. If the docs are correct and the implementation is wrong, fix the implementation and keep the docs aligned.
