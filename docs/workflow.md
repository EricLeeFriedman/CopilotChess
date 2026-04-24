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
- Automated review findings should stay high signal-to-noise, with inline comments used only when they map to a specific changed line.
- Automated review findings intended for Copilot follow-up — both the overall summary and every inline comment — should be prefixed with `@copilot`. These prefixes make each finding an explicit directed action item that the Copilot coding agent picks up automatically on its next pass.

## Documentation Expectations

- `AGENTS.md` is the map.
- The `docs\` directory is the deeper source of truth.
- Repository knowledge should live in versioned files, not only in chat or issue comments.

## Task Sizing

- Prefer short-lived tasks and pull requests.
- Split large efforts into documents, scaffolding, and implementation steps when possible.
- Preserve forward progress by writing down the next constraint instead of relying on memory.
