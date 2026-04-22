$ErrorActionPreference = 'Stop'

# Locate vswhere
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found. Install Visual Studio or Build Tools."
    exit 1
}

# Locate the latest VS installation with the x64 C++ toolchain
$installPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
$instanceId = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property instanceId

if (-not $installPath) {
    Write-Error "No Visual Studio installation with the C++ x64 toolchain was found."
    exit 1
}

$devShellDll = Join-Path $installPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
if (-not (Test-Path $devShellDll)) {
    Write-Error "Microsoft.VisualStudio.DevShell.dll not found at: $devShellDll"
    exit 1
}

Import-Module $devShellDll
Enter-VsDevShell $instanceId -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'

# Source and output paths
$repoRoot   = $PSScriptRoot
$sourceDir  = Join-Path $repoRoot 'src'
$buildDir   = Join-Path $repoRoot 'build'
$mainSource = Join-Path $sourceDir 'main.cpp'

if (-not (Test-Path $mainSource)) {
    Write-Error "src\main.cpp not found. Add the application entry point before building."
    exit 1
}

New-Item -Path $buildDir -ItemType Directory -Force | Out-Null
Push-Location $buildDir

$compileFlags = @(
    '/nologo',      # suppress compiler banner
    '/MT',          # statically link the runtime
    '/Gm-',         # disable minimal rebuild
    '/GR-',         # disable RTTI
    '/EHa-',        # disable exceptions
    '/Oi',          # enable intrinsic functions
    '/WX',          # warnings as errors
    '/W4',          # warning level 4
    '-std:c++20',   # C++ standard
    '-FC',          # full source path in diagnostics
    '-Zi'           # debug information
)

$linkerFlags = @(
    '/INCREMENTAL:NO'
)

$libs = @(
    'User32.lib',
    'Gdi32.lib'
)

cl @compileFlags $mainSource @libs /Fe:chess.exe /link @linkerFlags

$exitCode = $LASTEXITCODE
Pop-Location
exit $exitCode
