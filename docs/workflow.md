# Agent-First Workflow

This repository is designed so that work can move from intent to merged pull request with as little hidden context as possible.

## Work Intake

- Prefer GitHub issues for new work.
- Each issue should define the user-visible outcome, constraints touched, and acceptance criteria.
- If a task changes architecture or workflow assumptions, update the relevant docs before or alongside code.

## Change Flow

1. Read the issue and the repository entry documents.
2. Expand design notes in-repo when the task introduces new decisions.
3. Implement the smallest complete change.
4. Run the relevant validation path.
5. Self-review the diff for requirement alignment and architectural drift.
6. Open or update the pull request using the repository template.
7. Address review comments in follow-up commits until the task is complete.

## Review Expectations

- Agents should review their own work before asking for additional review.
- Pull requests should call out risks, validation, and any follow-up work explicitly.
- Review comments that represent durable rules should be promoted into repository docs or automation.
- Automated PR review is routed through `.github/workflows/pr-review.yml`, which installs GitHub Copilot CLI in Actions, invokes the repository's `.github/agents/cpp-pr-review.agent.md` custom agent, and converts its structured output into a GitHub pull request review.
- Reviews are posted using `GITHUB_TOKEN` so the structured machine review is always submitted successfully under `github-actions[bot]`.
- When a review requests changes, a separate "Ping Copilot" step posts a PR comment mentioning `@copilot` to trigger the coding agent for follow-up work. The ping includes the actual blocking findings from the review (path, line, and required fix) so the coding agent has all the context it needs without reconstructing it from PR history. This step only fires when the automated review has blocking findings (`REQUEST_CHANGES`) and uses `PERSONAL_ACCESS_TOKEN || GITHUB_TOKEN`; the workflow grants `issues: write` to `GITHUB_TOKEN` so the comment always succeeds without a PAT.
- The workflow enforces a hard cap of **7 blocking (`REQUEST_CHANGES`) automated review rounds** per pull request, counting only reviews authored by `github-actions[bot]`. Clean COMMENT reviews and human reviews do not count toward the cap. Once 7 blocking automated reviews have been posted, the workflow stops and posts a human-intervention notice via `GITHUB_TOKEN`.
- Automated review findings should stay high signal-to-noise, with inline comments used only when they map to a specific changed line.
- Automated review findings intended for Copilot follow-up — both the overall summary and every inline comment — should be prefixed with `@copilot`. These prefixes make each finding an explicit directed action item that the Copilot coding agent picks up automatically on its next pass.

### Review Routing by PR Type

The `pr-review.yml` workflow classifies every pull request before invoking any review agent:

- **Code PRs** (any file under `src/` or `build.ps1` is changed): the `cpp-pr-review` agent is invoked for full adversarial C/C++ review.
- **Process-only PRs** (only `docs/**`, `.github/**`, `AGENTS.md`, `README.md`, or other non-code files are changed): the `cpp-pr-review` agent is **skipped**. The workflow posts a single COMMENT review acknowledging the PR type and noting that the C/C++ review agent does not apply. Human or manual review of the process/docs changes is expected.
- **Mixed PRs** (both code and process files changed): treated as a code PR; `cpp-pr-review` runs in full.

This routing prevents the C/C++ specialist agent from producing irrelevant findings on workflow YAML, documentation, or template files — which was a primary source of review churn on automation PRs.

### Automation and Process Tasks

Use the **Workflow / Process / Automation** issue template (`.github/ISSUE_TEMPLATE/workflow.yml`) when opening issues for:

- Changes to GitHub Actions workflows (`.github/workflows/**`)
- Changes to agent prompts (`.github/agents/**`)
- Changes to issue templates, PR templates, or `AGENTS.md`
- Repository documentation changes that affect tooling expectations

The template requires explicit fields for:

- **Workflow trigger/event expectations**: which events, path filters, or conditions control when this workflow or process runs.
- **Auth/secrets/permissions**: tokens required, scopes needed, and what happens if a required secret is absent.
- **Output/format contract**: expected output schema or side-effects.
- **Fallback/error behavior**: what happens on failure or unexpected output.
- **Validation evidence**: how the change can be verified without relying solely on a live PR review round.

Automation tasks must describe their validation path up front. Acceptable validation approaches include:

1. Running `.github/scripts/validate-pr-review.py` locally or in CI to exercise parse and routing logic offline.
2. Opening a test PR that touches only `docs/` or `.github/` files and confirming the workflow posts an acknowledgment COMMENT (not `REQUEST_CHANGES`) and skips `cpp-pr-review`.
3. Script-based or fixture-based tests for any helper logic extracted from a workflow.

### Structured Review Output Schema

The review agent must return a JSON object with the following fields:

```json
{
  "event": "REQUEST_CHANGES" | "COMMENT" | "APPROVE",
  "body": "@copilot <overall summary>",
  "blocking": [
    {
      "path": "<file path relative to repo root>",
      "line": <line number in new file>,
      "body": "@copilot <required fix>"
    }
  ],
  "comments": [
    {
      "path": "<file path relative to repo root>",
      "line": <line number in new file>,
      "body": "@copilot <optional observation>"
    }
  ]
}
```

**`blocking`** is the machine-checkable list of required changes. Use it for:
- Real bugs
- Architecture or chess-rule violations
- Missing required tests
- Missing required doc updates

**`comments`** is for optional, non-blocking observations that do not require any change.

The workflow enforces the following rules regardless of the agent's top-level `event` field:
1. If `blocking` is non-empty → the review is posted as `REQUEST_CHANGES`.
2. If `blocking` is empty and `event` is `REQUEST_CHANGES` → the review is still posted as `REQUEST_CHANGES` (conservative: a mismatch is logged as a warning and the blocking direction is kept).
3. If the agent output cannot be parsed → the review is posted as `REQUEST_CHANGES` with an error message instead of silently passing as `COMMENT`.
4. `COMMENT` is reserved for reviews where `blocking` is empty and `event` is `COMMENT` or `APPROVE` (approvals are converted to `COMMENT`).

This means a mismatch — where the agent body describes real bugs but sets `event: "COMMENT"` — can no longer silently slip through as a non-blocking review.

## Retrospective Workflow

A dedicated **Retrospective Analysis** workflow (`.github/workflows/retrospective.yml`) runs a meta-improvement pass over a merged or closed pull request to identify why the automated system struggled and to propose concrete improvements.

- The workflow is triggered in two ways:
  - **Manual trigger (`workflow_dispatch`)**: provide any pull request number. The retrospective always runs.
  - **Automatic trigger (`pull_request: closed`)**: runs whenever a pull request is closed within the same repository, but only proceeds when the closed PR accumulated **three or more** automated `REQUEST_CHANGES` review rounds from `github-actions[bot]`. PRs with fewer rounds are skipped automatically.
- The workflow collects the full PR history: metadata, commit list, all reviews, all issue comments, all inline review comments, and the PR diff. This data is assembled into `retro-input.md` for the agent.
- The `.github/agents/retrospective.agent.md` agent reads the collected history alongside the system context files (`AGENTS.md`, workflow docs, the `pr-review.yml` workflow, and the `cpp-pr-review` agent) to produce a structured retrospective report.
- The report classifies findings as one-off mistakes, repeated/systemic mistakes, or missing safeguards; identifies root causes; and proposes prioritized, file-specific improvements to agent prompts, workflow logic, docs, and templates.
- The report is posted as a PR comment under `github-actions[bot]` using `GITHUB_TOKEN`. A duplicate-check sentinel prevents the same comment from being posted more than once per PR.
- Like the PR review workflow, this workflow requires a `COPILOT_REVIEW_TOKEN` or `PERSONAL_ACCESS_TOKEN` repository secret for the Copilot CLI agent step.

## Documentation Expectations

- `AGENTS.md` is the map.
- The `docs\` directory is the deeper source of truth.
- Repository knowledge should live in versioned files, not only in chat or issue comments.

## Task Sizing

- Prefer short-lived tasks and pull requests.
- Split large efforts into documents, scaffolding, and implementation steps when possible.
- Preserve forward progress by writing down the next constraint instead of relying on memory.
