[CmdletBinding()]
param(
    [ValidateRange(0, 99)]
    [int]$Major = 4,

    [ValidateRange(0, 99)]
    [int]$Minor = 6
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$script:LogPath = ""

function Write-Logged {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    Write-Output $Message
    if (-not [string]::IsNullOrWhiteSpace($script:LogPath)) {
        Add-Content -LiteralPath $script:LogPath -Value $Message -Encoding UTF8
    }
}

# Windows PowerShell 5.1 wraps every native stderr line in a NativeCommandError
# record, so $ErrorActionPreference = "Stop" aborts on a benign diagnostic even
# when the process exits 0. The engine writes every OpenGL debug message to
# stderr by design (src/platform/window_runtime.cpp), so lower the preference
# around the invocation only, keep the merged text as evidence, and grade the
# command by its exit code. See QA-009 in docs/qa/.
function Invoke-NativeMerged {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [AllowEmptyCollection()]
        [string[]]$Arguments = @()
    )

    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $merged = & $FilePath @Arguments 2>&1
    }
    finally {
        $ErrorActionPreference = $previousPreference
    }
    $exitCode = $LASTEXITCODE
    return [pscustomobject]@{
        ExitCode = $exitCode
        Lines = [string[]]@($merged | ForEach-Object { $_.ToString() })
    }
}

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Label,

        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$NativeArguments
    )

    Write-Logged -Message ("command={0} {1}" -f $FilePath, ($NativeArguments -join " "))
    $invocation = Invoke-NativeMerged -FilePath $FilePath -Arguments $NativeArguments
    foreach ($line in $invocation.Lines) {
        Write-Logged -Message $line
    }
    $nativeExitCode = $invocation.ExitCode
    if ($nativeExitCode -ne 0) {
        throw "$Label failed with exit code $nativeExitCode."
    }
}

function Get-VisualStudioInstallation {
    $installerRoot = ${env:ProgramFiles(x86)}
    if ([string]::IsNullOrWhiteSpace($installerRoot)) {
        throw "ProgramFiles(x86) is unavailable; this script requires 64-bit Windows."
    }

    $vswhere = Join-Path $installerRoot "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "Visual Studio Build Tools were not found. See docs/setup/WINDOWS.md."
    }

    $installationPath = & $vswhere `
        -latest `
        -products "*" `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
        throw "A Visual Studio installation with the x64 C++ toolset was not found."
    }

    return $installationPath.Trim()
}

function Import-VisualStudioEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$InstallationPath
    )

    $vsDevCmd = Join-Path $InstallationPath "Common7\Tools\VsDevCmd.bat"
    if (-not (Test-Path -LiteralPath $vsDevCmd -PathType Leaf)) {
        throw "VsDevCmd.bat was not found under $InstallationPath."
    }

    $environmentCommand = '"{0}" -no_logo -arch=x64 -host_arch=x64 >nul && set' -f $vsDevCmd
    $environmentExitCode = 2
    Push-Location -LiteralPath $env:SystemRoot
    try {
        $environmentLines = & $env:COMSPEC /d /s /c $environmentCommand
        $environmentExitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
    if ($environmentExitCode -ne 0) {
        throw "VsDevCmd.bat failed with exit code $environmentExitCode."
    }

    foreach ($line in $environmentLines) {
        $separator = $line.IndexOf("=")
        if ($separator -le 0) {
            continue
        }

        $name = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
}

function Resolve-RequiredTool {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$BundledPath
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }
    if (-not [string]::IsNullOrWhiteSpace($BundledPath) -and
        (Test-Path -LiteralPath $BundledPath -PathType Leaf)) {
        return $BundledPath
    }

    throw "$Name was not found. Modify Visual Studio Build Tools and include the recommended C++ components."
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$contextSource = Join-Path $PSScriptRoot "context-smoke"
$artifactDirectory = Join-Path $repoRoot ("artifacts\phase0\{0}" -f (Get-Date -Format "yyyy-MM-dd"))
$localRoot = Join-Path $env:LOCALAPPDATA "WideEye\phase0"
$localSource = Join-Path $localRoot "context-smoke-source"
$localBuild = Join-Path $localRoot "context-smoke-build"

New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null

$logTimestamp = Get-Date -Format "HHmmssfff"
$logPath = Join-Path $artifactDirectory ("windows-context-smoke-{0}.log" -f $logTimestamp)
$script:LogPath = $logPath
Set-Content -LiteralPath $logPath -Value "wide_eye_phase0_windows_context_smoke" -Encoding UTF8
$scriptExitCode = 2

try {
    $installationPath = Get-VisualStudioInstallation
    Import-VisualStudioEnvironment -InstallationPath $installationPath

    $cmakeBundled = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ninjaBundled = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    $cmake = Resolve-RequiredTool -Name "cmake.exe" -BundledPath $cmakeBundled
    $ninja = Resolve-RequiredTool -Name "ninja.exe" -BundledPath $ninjaBundled
    $compiler = Resolve-RequiredTool -Name "cl.exe" -BundledPath ""

    New-Item -ItemType Directory -Force -Path $localSource | Out-Null
    New-Item -ItemType Directory -Force -Path $localBuild | Out-Null
    Copy-Item -LiteralPath (Join-Path $contextSource "CMakeLists.txt") -Destination $localSource -Force
    Copy-Item -LiteralPath (Join-Path $contextSource "main.cpp") -Destination $localSource -Force

    $operatingSystem = Get-CimInstance Win32_OperatingSystem
    Write-Logged -Message "platform=native-windows"
    Write-Logged -Message ("os_caption={0}" -f $operatingSystem.Caption)
    Write-Logged -Message ("os_version={0}" -f $operatingSystem.Version)
    Write-Logged -Message ("os_build={0}" -f $operatingSystem.BuildNumber)
    Write-Logged -Message ("os_architecture={0}" -f $operatingSystem.OSArchitecture)
    Write-Logged -Message ("visual_studio={0}" -f $installationPath)
    Write-Logged -Message ("cmake={0}" -f $cmake)
    Write-Logged -Message ("ninja={0}" -f $ninja)
    Write-Logged -Message ("compiler={0}" -f $compiler)
    Write-Logged -Message ("local_build={0}" -f $localBuild)
    Write-Logged -Message ("artifact_log={0}" -f $logPath)

    $gpuIndex = 0
    foreach ($gpu in (Get-CimInstance Win32_VideoController)) {
        Write-Logged -Message ("gpu_{0}_name={1}" -f $gpuIndex, $gpu.Name)
        Write-Logged -Message ("gpu_{0}_driver={1}" -f $gpuIndex, $gpu.DriverVersion)
        $gpuIndex += 1
    }

    foreach ($sourceName in "CMakeLists.txt", "main.cpp") {
        $sourcePath = Join-Path $contextSource $sourceName
        $sourceHash = Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256
        Write-Logged -Message ("source_sha256_{0}={1}" -f $sourceName, $sourceHash.Hash.ToLowerInvariant())
    }

    Invoke-CheckedNative -Label "CMake version query" -FilePath $cmake -NativeArguments @("--version")
    Invoke-CheckedNative -Label "Ninja version query" -FilePath $ninja -NativeArguments @("--version")
    Invoke-CheckedNative -Label "CMake configure" -FilePath $cmake -NativeArguments @(
        "-S", $localSource,
        "-B", $localBuild,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_MAKE_PROGRAM=$ninja",
        "-DCMAKE_C_COMPILER=$compiler",
        "-DCMAKE_CXX_COMPILER=$compiler"
    )
    Invoke-CheckedNative -Label "CMake build" -FilePath $cmake -NativeArguments @(
        "--build", $localBuild, "--parallel"
    )

    $executable = Join-Path $localBuild "wide-eye-context-smoke.exe"
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "The expected diagnostic executable was not produced at $executable."
    }

    Write-Logged -Message ("smoke_executable={0}" -f $executable)
    Write-Logged -Message ("requested_gl={0}.{1}" -f $Major, $Minor)
    $smoke = Invoke-NativeMerged -FilePath $executable -Arguments @("$Major", "$Minor")
    foreach ($line in $smoke.Lines) {
        Write-Logged -Message $line
    }
    $scriptExitCode = $smoke.ExitCode
    Write-Logged -Message ("smoke_exit_code={0}" -f $scriptExitCode)
}
catch {
    $failureLines = @(
        "result=fail",
        "failure_stage=windows_setup_or_build",
        ("error={0}" -f $_.Exception.Message)
    )
    foreach ($line in $failureLines) {
        [Console]::Error.WriteLine($line)
        Add-Content -LiteralPath $script:LogPath -Value $line -Encoding UTF8
    }
    $scriptExitCode = 2
}

exit $scriptExitCode
