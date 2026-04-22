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

- Every pull request receives an automated adversarial review from the `pr-review.yml` workflow before a human looks at it. The reviewer acts as a skeptical expert, checking for bugs, constraint violations, and missing doc updates. It posts a formal GitHub PR review (request-changes / comment / approve) with inline comments where possible.
- The automated review caps diffs at 12 000 characters to stay within model token limits. For large PRs the reviewer may see only the first portion of the diff; contributors should note this in the PR description when a PR is unusually large.
- All automated review comments are prefixed with `@copilot` so agents automatically pick them up as action items on the next pass. Agents should treat each `@copilot`-prefixed finding as an explicit request to investigate and, if valid, fix the issue in a follow-up commit.
- Agents should review their own work before asking for additional review.
- Pull requests should call out risks, validation, and any follow-up work explicitly.
- Review comments that represent durable rules should be promoted into repository docs or automation.

## Documentation Expectations

- `AGENTS.md` is the map.
- The `docs\` directory is the deeper source of truth.
- Repository knowledge should live in versioned files, not only in chat or issue comments.

## Task Sizing

- Prefer short-lived tasks and pull requests.
- Split large efforts into documents, scaffolding, and implementation steps when possible.
- Preserve forward progress by writing down the next constraint instead of relying on memory.
