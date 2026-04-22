# Build And Runner Notes

## CI Workflows

| Workflow file | Runner | Trigger | Purpose |
|---|---|---|---|
| `.github/workflows/build.yml` | `windows-latest` | push / pull_request to `main` | Compile with `build.ps1` and run `chess.exe --test` |
| `.github/workflows/pr-review.yml` | `ubuntu-latest` | pull_request (opened, synchronize, reopened) | Adversarial LLM code review posted as a formal GitHub PR review |

The `pr-review.yml` workflow uses the GitHub Models API (`gpt-4o`) with the built-in `GITHUB_TOKEN` — no external secrets are required. It loads `.github/copilot-instructions.md` and `AGENTS.md` as review context, then posts a request-changes / comment / approve review with inline comments on the PR.

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
