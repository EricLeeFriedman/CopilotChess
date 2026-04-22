# Build And Runner Notes

## Intended Build Entry Point

The product build should ultimately be driven by a simple `build.ps1` script that invokes `cl.exe`.

That script is intentionally **not** introduced in the initial repository commit because this stage is reserved for agent workflow scaffolding and documentation only.

## Copilot Cloud-Agent Target

This repository is currently configured for a **self-hosted Windows x64 runner**.

The workflow file uses the default self-hosted label set:

- `self-hosted`
- `Windows`
- `X64`

## Runner Requirements

The self-hosted runner should provide:

- Windows x64
- PowerShell
- Git
- Visual Studio Build Tools or Visual Studio with the C++ x64 toolchain installed
- `vswhere.exe`

## Building Locally Or In CI

```
.\build.ps1
```

`build.ps1` is fully self-contained. It locates Visual Studio via `vswhere.exe`, imports the `Microsoft.VisualStudio.DevShell` module, calls `Enter-VsDevShell` to configure the x64 toolchain, and then invokes `cl.exe` on `src\main.cpp`. Output goes to `build\`.

No separate environment setup step is required — you can call `build.ps1` from any PowerShell terminal that has Visual Studio installed.

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
