# Build And Runner Notes

## Build Entry Point

```
.\build.ps1
```

This repository uses the **`windows-latest` GitHub-hosted runner**.

## Runner Requirements

The `windows-latest` runner provides everything needed out of the box:

- Windows x64
- PowerShell
- Git
- Visual Studio Build Tools with the C++ x64 toolchain
- `vswhere.exe`

## Building Locally Or In CI

```
.\build.ps1
```

`build.ps1` is fully self-contained. It locates Visual Studio via `vswhere.exe`, imports the `Microsoft.VisualStudio.DevShell` module, calls `Enter-VsDevShell` to configure the x64 toolchain, and then invokes `cl.exe` on all `*.cpp` files found in `src\`. Output goes to `build\`.

Every `.cpp` file in `src\` is compiled and linked into the single `chess.exe` binary. No separate environment setup step is required — you can call `build.ps1` from any PowerShell terminal that has Visual Studio installed.

## Compiler Flags

The build uses these flags (defined in `build.ps1`):

| Flag | Purpose |
|------|---------|
| `/MT` | Statically link the runtime |
| `/GR-` | Disable RTTI |
| `/EHa-` | Disable exceptions |
| `/WX /W4` | Treat all warnings as errors at level 4 |
| `-std:c++20` | C++20 standard |
| `-Zi` | Debug information |
| `/INCREMENTAL:NO` | Deterministic full links |

## Environment Preparation

The Copilot setup workflow locates the latest installed Visual Studio toolchain, imports `Microsoft.VisualStudio.DevShell.dll`, calls `Enter-VsDevShell`, and exports the relevant MSVC environment variables to `GITHUB_ENV` so `cl.exe` is available in subsequent agent steps.

## Administrative Note

GitHub Copilot cloud-agent Windows environments require repository administrators to use compatible network controls for the self-hosted runner. The repository commit can prepare the workflow file, but runner registration and repository security settings must still be handled outside the repository.

## Workflows

The primary GitHub Actions workflow for code validation is:

| Workflow | File | Runner | Purpose |
|---|---|---|---|
| Build | `.github/workflows/build.yml` | `windows-latest` | Compile and run the built-in test suite on every push/PR targeting `main`. Requires the Windows MSVC toolchain. |
| PR Adversarial Review | `.github/workflows/pr-review.yml` | `ubuntu-latest` | Installs GitHub Copilot CLI, invokes the repository's `cpp-pr-review` custom agent against the PR diff, and posts the resulting GitHub review for same-repository pull requests. The Copilot agent step requires either a `COPILOT_REVIEW_TOKEN` or `PERSONAL_ACCESS_TOKEN` repository secret containing a fine-grained PAT with `Copilot Requests` permission. The workflow grants `pull-requests: write` and `issues: write` to `GITHUB_TOKEN` so both the review submission and the PR issue-comment steps always succeed without a PAT. When the review requests changes (`REQUEST_CHANGES`), a separate "Ping Copilot" step posts a PR comment mentioning `@copilot` using `PERSONAL_ACCESS_TOKEN || GITHUB_TOKEN`. The workflow enforces a hard limit of 7 `REQUEST_CHANGES` automated review rounds per pull request, counting only reviews authored by `github-actions[bot]`; once that limit is reached it posts a human-intervention notice via `GITHUB_TOKEN` and skips further automated reviews. |
| Retrospective Analysis | `.github/workflows/retrospective.yml` | `ubuntu-latest` | Performs a meta-improvement retrospective over a closed or merged pull request. Triggered manually via `workflow_dispatch` (any PR number) or automatically on `pull_request: closed` when the PR accumulated three or more automated blocking review rounds. Collects PR metadata, commit history, all reviews, all comments, and the diff into `retro-input.md`, then invokes the `retrospective` custom agent to identify repeated failure patterns, root causes, and prioritized improvements to agent prompts, workflow logic, docs, and templates. Posts the report as a PR comment under `github-actions[bot]` using `GITHUB_TOKEN`. Requires the same `COPILOT_REVIEW_TOKEN` or `PERSONAL_ACCESS_TOKEN` secret as the PR review workflow. |
