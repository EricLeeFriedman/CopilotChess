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

## Environment Preparation

The Copilot setup workflow locates the latest installed Visual Studio toolchain, runs `VsDevCmd.bat`, and exports the relevant MSVC environment variables so the agent session can discover `cl.exe`, headers, and libraries consistently.

## Administrative Note

GitHub Copilot cloud-agent Windows environments require repository administrators to use compatible network controls for the self-hosted runner. The repository commit can prepare the workflow file, but runner registration and repository security settings must still be handled outside the repository.
