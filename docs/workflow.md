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
- When a review requests changes, a separate "Ping Copilot" step posts a PR comment mentioning `@copilot` to trigger the coding agent for follow-up work. This step only fires when the automated review has blocking findings (`REQUEST_CHANGES`) and uses `PERSONAL_ACCESS_TOKEN || GITHUB_TOKEN`; the workflow grants `issues: write` to `GITHUB_TOKEN` so the comment always succeeds without a PAT.
- The workflow enforces a hard cap of **7 blocking (`REQUEST_CHANGES`) automated review rounds** per pull request, counting only reviews authored by `github-actions[bot]`. Clean COMMENT reviews and human reviews do not count toward the cap. Once 7 blocking automated reviews have been posted, the workflow stops and posts a human-intervention notice via `GITHUB_TOKEN`.
- Automated review findings should stay high signal-to-noise, with inline comments used only when they map to a specific changed line.
- Automated review findings intended for Copilot follow-up — both the overall summary and every inline comment — should be prefixed with `@copilot`. These prefixes make each finding an explicit directed action item that the Copilot coding agent picks up automatically on its next pass.

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
