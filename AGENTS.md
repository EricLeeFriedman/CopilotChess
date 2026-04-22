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

## When Docs and Code Disagree

Fix the disagreement in the same change if possible. If the implementation is correct and the docs are stale, update the docs. If the docs are correct and the implementation is wrong, fix the implementation and keep the docs aligned.
