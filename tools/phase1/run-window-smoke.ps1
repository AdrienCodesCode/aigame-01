[CmdletBinding()]
param(
    [switch]$InjectCaptureMismatch
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:CommandRecords = [System.Collections.Generic.List[object]]::new()
$script:ObservedState = [ordered]@{
    scenario = "voxel_cube_smoke"
    scenario_version = 1
    replay = $null
    replay_version = $null
    seed = $null
    simulation_tick = $null
    simulation_rate_hz = 60
    camera = "tracer0_fixed_perspective"
    graphics_profile = "development"
}
$script:CurrentStage = "initialization"
$script:LogPath = ""

function Write-Logged {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    [Console]::Out.WriteLine($Message)
    if (-not [string]::IsNullOrWhiteSpace($script:LogPath)) {
        Add-Content -LiteralPath $script:LogPath -Value $Message -Encoding UTF8
    }
}

function Write-JsonFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [object]$Value,

        [int]$Depth = 10
    )

    $json = $Value | ConvertTo-Json -Depth $Depth
    $utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $utf8WithoutBom)
}

function Add-ObservedState {
    param(
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string[]]$Lines
    )

    foreach ($line in $Lines) {
        if ($line -match "^([A-Za-z][A-Za-z0-9_.-]*)=(.*)$") {
            $script:ObservedState[$Matches[1]] = $Matches[2]
        }
    }
}

function Format-Command {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$NativeArguments
    )

    $formattedArguments = foreach ($argument in $NativeArguments) {
        if ($argument -match '[\s"]') {
            '"{0}"' -f ($argument -replace '"', '\"')
        }
        else {
            $argument
        }
    }
    return (@($FilePath) + $formattedArguments) -join " "
}

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Stage,

        [Parameter(Mandatory = $true)]
        [string]$Label,

        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$NativeArguments,

        [switch]$RecordState
    )

    $script:CurrentStage = $Stage
    $commandLine = Format-Command -FilePath $FilePath -NativeArguments $NativeArguments
    Write-Logged -Message ("command={0}" -f $commandLine)

    $startedAt = (Get-Date).ToUniversalTime()
    $capturedLines = [System.Collections.Generic.List[string]]::new()
    $nativeExitCode = -1
    $invocationError = $null
    try {
        & $FilePath @NativeArguments 2>&1 | ForEach-Object {
            $line = $_.ToString()
            [void]$capturedLines.Add($line)
            Write-Logged -Message $line
        }
        $nativeExitCode = $LASTEXITCODE
    }
    catch {
        $invocationError = $_.Exception.Message
        Write-Logged -Message ("native_invocation_error={0}" -f $invocationError)
    }

    $finishedAt = (Get-Date).ToUniversalTime()
    $record = [ordered]@{
        stage = $Stage
        label = $Label
        file = $FilePath
        arguments = @($NativeArguments)
        command = $commandLine
        started_at_utc = $startedAt.ToString("o")
        finished_at_utc = $finishedAt.ToString("o")
        exit_code = $nativeExitCode
        result = if ($nativeExitCode -eq 0) { "pass" } else { "fail" }
    }
    if ($null -ne $invocationError) {
        $record.error = $invocationError
    }
    [void]$script:CommandRecords.Add([pscustomobject]$record)

    $lineArray = [string[]]$capturedLines.ToArray()
    if ($RecordState) {
        Add-ObservedState -Lines $lineArray
    }
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

        [Parameter(Mandatory = $true)]
        [string]$BundledPath
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }
    if (Test-Path -LiteralPath $BundledPath -PathType Leaf) {
        return $BundledPath
    }

    throw "$Name was not found. Modify Visual Studio Build Tools and include the recommended C++ components."
}

function Get-SourceState {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepositoryRoot
    )

    $sourceState = [ordered]@{
        commit = $null
        worktree_state = "unknown"
        status = @()
    }
    $git = Get-Command "git.exe" -ErrorAction SilentlyContinue
    if ($null -eq $git) {
        return [pscustomobject]$sourceState
    }

    try {
        $safeRepositoryRoot = $RepositoryRoot.Replace("\", "/")
        $commit = & $git.Source -c "safe.directory=$safeRepositoryRoot" -C $RepositoryRoot rev-parse HEAD 2>$null
        if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($commit)) {
            return [pscustomobject]$sourceState
        }

        $status = @(
            & $git.Source `
                -c "safe.directory=$safeRepositoryRoot" `
                -C $RepositoryRoot `
                status --short --untracked-files=normal 2>$null
        )
        if ($LASTEXITCODE -ne 0) {
            return [pscustomobject]$sourceState
        }

        $sourceState.commit = $commit.Trim()
        $sourceState.worktree_state = if ($status.Count -eq 0) { "clean" } else { "dirty" }
        $sourceState.status = @($status | ForEach-Object { $_.ToString() })
    }
    catch {
        $sourceState.worktree_state = "unknown"
    }
    return [pscustomobject]$sourceState
}

function Get-PlatformState {
    $platformState = [ordered]@{
        name = "native-windows"
        os = [Environment]::OSVersion.VersionString
        architecture = $env:PROCESSOR_ARCHITECTURE
        cpu = $null
        gpus = @()
    }

    try {
        $operatingSystem = Get-CimInstance -ClassName Win32_OperatingSystem
        $platformState.os = "{0} {1} build {2}" -f $operatingSystem.Caption, $operatingSystem.Version, $operatingSystem.BuildNumber
    }
    catch {
        Write-Logged -Message ("metadata_warning=os_query_failed error={0}" -f $_.Exception.Message)
    }
    try {
        $processor = Get-CimInstance -ClassName Win32_Processor | Select-Object -First 1
        $platformState.cpu = $processor.Name.Trim()
    }
    catch {
        Write-Logged -Message ("metadata_warning=cpu_query_failed error={0}" -f $_.Exception.Message)
    }
    try {
        $platformState.gpus = @(
            Get-CimInstance -ClassName Win32_VideoController | ForEach-Object {
                [pscustomobject][ordered]@{
                    name = $_.Name
                    driver_version = $_.DriverVersion
                }
            }
        )
    }
    catch {
        Write-Logged -Message ("metadata_warning=gpu_query_failed error={0}" -f $_.Exception.Message)
    }
    return [pscustomobject]$platformState
}

function Get-ToolVersion {
    param(
        [AllowNull()]
        [string]$FilePath
    )

    if ([string]::IsNullOrWhiteSpace($FilePath)) {
        return $null
    }
    try {
        $versionLines = @(& $FilePath --version 2>$null)
        if ($LASTEXITCODE -eq 0 -and $versionLines.Count -gt 0) {
            return $versionLines[0].ToString().Trim()
        }
    }
    catch {
        return $null
    }
    return $null
}

function Get-StringSha256 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
        return ([System.BitConverter]::ToString($algorithm.ComputeHash($bytes))).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Add-ArtifactRecord {
    param(
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.List[object]]$Records,

        [Parameter(Mandatory = $true)]
        [string]$Role,

        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$MediaType
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return
    }
    $hash = Get-FileHash -LiteralPath $Path -Algorithm SHA256
    $file = Get-Item -LiteralPath $Path
    [void]$Records.Add([pscustomobject][ordered]@{
        role = $Role
        path = $file.Name
        media_type = $MediaType
        bytes = $file.Length
        sha256 = $hash.Hash.ToLowerInvariant()
    })
}

function Write-ArtifactPacket {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PacketDirectory,

        [Parameter(Mandatory = $true)]
        [string]$ManifestPath,

        [Parameter(Mandatory = $true)]
        [string]$ConfigurationPath,

        [Parameter(Mandatory = $true)]
        [string]$StatePath,

        [Parameter(Mandatory = $true)]
        [string]$SourceHashesPath,

        [Parameter(Mandatory = $true)]
        [object]$SourceState,

        [Parameter(Mandatory = $true)]
        [object]$PlatformState,

        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]
        [object[]]$SourceHashes,

        [Parameter(Mandatory = $true)]
        [string]$Result,

        [AllowNull()]
        [string]$FailureStage,

        [AllowNull()]
        [string]$FailureMessage,

        [AllowNull()]
        [string]$CMakePath,

        [AllowNull()]
        [string]$CTestPath,

        [Parameter(Mandatory = $true)]
        [string]$LocalSource,

        [Parameter(Mandatory = $true)]
        [string]$NormalCapturePath,

        [Parameter(Mandatory = $true)]
        [string]$RepeatCapturePath,

        [Parameter(Mandatory = $true)]
        [string]$DebugCapturePath,

        [Parameter(Mandatory = $true)]
        [datetime]$StartedAt
    )

    $ctestCaptureNames = @("voxel-cube-capture-one.png", "voxel-cube-capture-two.png")
    foreach ($captureName in $ctestCaptureNames) {
        $temporaryCapture = Join-Path $LocalSource ("build\Windows\dev\{0}" -f $captureName)
        if (Test-Path -LiteralPath $temporaryCapture -PathType Leaf) {
            Copy-Item -LiteralPath $temporaryCapture -Destination (Join-Path $PacketDirectory ("ctest-{0}" -f $captureName)) -Force
        }
    }

    $sourceInventory = [ordered]@{
        schema_version = 1
        aggregate_sha256 = Get-StringSha256 -Value (($SourceHashes | ForEach-Object { "{0}={1}" -f $_.path, $_.sha256 }) -join "`n")
        files = @($SourceHashes)
    }
    Write-JsonFile -Path $SourceHashesPath -Value $sourceInventory -Depth 6

    $configuration = [ordered]@{
        schema_version = 1
        configure_preset = "dev"
        build_preset = "dev"
        test_preset = "dev"
        cmake = [ordered]@{
            path = $CMakePath
            version = Get-ToolVersion -FilePath $CMakePath
        }
        ctest = [ordered]@{
            path = $CTestPath
            version = Get-ToolVersion -FilePath $CTestPath
        }
        required_opengl = "4.6 Core debug"
        required_glsl = "4.60"
        viewport = [ordered]@{ width = 64; height = 64 }
        capture_format = "png_rgba8_top_left"
        debug_view = "same-camera triangle wireframe"
        injected_capture_mismatch = $InjectCaptureMismatch.IsPresent
    }
    Write-JsonFile -Path $ConfigurationPath -Value $configuration -Depth 6
    Write-JsonFile -Path $StatePath -Value ([pscustomobject]$script:ObservedState) -Depth 6

    Add-Content `
        -LiteralPath $script:LogPath `
        -Value ("artifact_manifest={0}" -f $ManifestPath) `
        -Encoding UTF8
    $artifactRecords = [System.Collections.Generic.List[object]]::new()
    Add-ArtifactRecord -Records $artifactRecords -Role "log" -Path $script:LogPath -MediaType "text/plain"
    Add-ArtifactRecord -Records $artifactRecords -Role "configuration" -Path $ConfigurationPath -MediaType "application/json"
    Add-ArtifactRecord -Records $artifactRecords -Role "state" -Path $StatePath -MediaType "application/json"
    Add-ArtifactRecord -Records $artifactRecords -Role "source_hashes" -Path $SourceHashesPath -MediaType "application/json"
    Add-ArtifactRecord -Records $artifactRecords -Role "normal_capture" -Path $NormalCapturePath -MediaType "image/png"
    Add-ArtifactRecord -Records $artifactRecords -Role "repeat_capture" -Path $RepeatCapturePath -MediaType "image/png"
    Add-ArtifactRecord -Records $artifactRecords -Role "debug_capture" -Path $DebugCapturePath -MediaType "image/png"
    foreach ($captureName in $ctestCaptureNames) {
        Add-ArtifactRecord `
            -Records $artifactRecords `
            -Role "failed_ctest_capture" `
            -Path (Join-Path $PacketDirectory ("ctest-{0}" -f $captureName)) `
            -MediaType "image/png"
    }

    $finishedAt = (Get-Date).ToUniversalTime()
    $failure = $null
    if ($Result -eq "fail") {
        $failure = [ordered]@{
            stage = $FailureStage
            message = $FailureMessage
            capture_available = (Test-Path -LiteralPath $NormalCapturePath -PathType Leaf)
            debug_capture_available = (Test-Path -LiteralPath $DebugCapturePath -PathType Leaf)
        }
    }
    $reproductionCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    if ($InjectCaptureMismatch.IsPresent) {
        $reproductionCommand += " -InjectCaptureMismatch"
    }
    $manifest = [ordered]@{
        schema = "wide-eye.artifact-manifest"
        schema_version = 1
        packet_version = "tracer0-cube-review-v1"
        result = $Result
        started_at_utc = $StartedAt.ToUniversalTime().ToString("o")
        finished_at_utc = $finishedAt.ToString("o")
        source = $SourceState
        platform = $PlatformState
        executable = [ordered]@{
            name = "wide_eye"
            version = "0.1.0"
        }
        scenario = [ordered]@{
            name = "voxel_cube_smoke"
            version = 1
            replay = $null
            seed = $null
            simulation_tick = $null
            simulation_rate_hz = 60
            camera = "tracer0_fixed_perspective"
            viewport = [ordered]@{ width = 64; height = 64 }
            graphics_profile = "development"
            flags = @("--voxel-cube-smoke --capture", "--voxel-cube-debug-smoke --capture")
        }
        reproduction_command = $reproductionCommand
        review_document = "review.md"
        commands = @($script:CommandRecords | ForEach-Object { $_ })
        failure = $failure
        artifacts = @($artifactRecords | ForEach-Object { $_ })
    }
    Write-JsonFile -Path $ManifestPath -Value $manifest
    [Console]::Out.WriteLine("artifact_manifest={0}" -f $ManifestPath)
}

function Write-VisualReviewPacket {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ReviewPath,

        [Parameter(Mandatory = $true)]
        [string]$ManifestPath,

        [Parameter(Mandatory = $true)]
        [object]$SourceState,

        [Parameter(Mandatory = $true)]
        [object]$PlatformState,

        [Parameter(Mandatory = $true)]
        [datetime]$StartedAt
    )

    $manifestHash = (Get-FileHash -LiteralPath $ManifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $gpuSummary = ($PlatformState.gpus | ForEach-Object { "{0} driver {1}" -f $_.name, $_.driver_version }) -join "; "
    if ([string]::IsNullOrWhiteSpace($gpuSummary)) {
        $gpuSummary = "unavailable"
    }
    $sourceSummary = "{0}; worktree {1}" -f $SourceState.commit, $SourceState.worktree_state
    $glSummary = "{0}; GLSL {1}" -f $script:ObservedState["gl_version"], $script:ObservedState["glsl_version"]
    $reproductionCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    $review = @"
# Tracer 0 candidate cube visual review

This is a candidate review packet, not an accepted golden. The owner verdict is intentionally blank.

## Review question

- **Coherent outcome:** Reproducibly render and explain the Tracer 0 perspective voxel cube on the approved native Windows OpenGL 4.6 path.
- **Question for the owner:** Is the colored perspective cube, together with its matching wireframe diagnostic, acceptable as the first visual baseline for Tracer 0?
- **What intentionally changed:** The normal cube presentation is unchanged. A same-camera wireframe diagnostic and this review record were added.
- **What must remain invariant:** The 64x64 viewport, fixed camera, cube geometry, normal face colors, depth test/write state, deterministic PNG capture, and zero high-severity GL result.
- **Known limitations or unverified claims:** This is a static engineering tracer at 64x64, not representative game art. The wireframe exposes submitted triangle edges, including face diagonals; it is not a voxel/chunk or collision view. Motion and performance are outside this review question. There is no prior accepted baseline, and native Linux remains unverified.

## Reproduction metadata

| Field | Value |
| --- | --- |
| Date/time and timezone | $($StartedAt.ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss zzz")) |
| Commit and dirty-worktree state | $sourceSummary |
| Configure/build preset | dev / dev |
| Executable version | Wide Eye 0.1.0 |
| OS and architecture | $($PlatformState.os); $($PlatformState.architecture) |
| CPU | $($PlatformState.cpu) |
| GPU and driver | $gpuSummary; active renderer $($script:ObservedState["gl_renderer"]) |
| OpenGL and GLSL versions | $glSummary |
| Scenario and version | voxel_cube_smoke, version 1 |
| Replay and version | Not applicable; static smoke scenario |
| Seed | Not applicable |
| Simulation tick/rate | No simulation tick; fixed-step configuration 60 Hz |
| Camera and viewport | tracer0_fixed_perspective; 64x64 |
| Graphics profile and relevant flags | development; normal and wireframe_debug captures |
| Exact reproduction command | $reproductionCommand |
| Artifact-manifest path/hash | manifest.json / $manifestHash |

## Automated evidence

| Check or budget | Command/configuration | Result | Evidence path |
| --- | --- | --- | --- |
| Focused tests | ctest --preset dev | Pass | run.log |
| Scenario/replay | Normal and wireframe debug capture commands | Pass; matching scenario/camera/viewport | normal-frame.png, debug-frame.png, state.json |
| GL diagnostics | OpenGL 4.6 Core debug callback | Pass; zero high-severity messages | run.log, state.json |
| Sanitizers, if required | Not run by this native Windows packet | Intentionally separate; WSL sanitizer coverage cannot execute this host's GL 4.6 path | Not included |
| Frame-time/memory, if required | Not required for this static Tracer 0 visual question | Not run | Not included |

## Artifacts

| Artifact | Required when | Path | What to inspect |
| --- | --- | --- | --- |
| Normal frame | Static presentation matters | normal-frame.png | Perspective, silhouette, three readable colored faces, dark clear color |
| Matching debug frame | Geometry diagnosis matters | debug-frame.png | Same silhouette/camera and visible triangle edges, including face diagonals |
| Short clip or contact sheet | Motion/temporal behavior matters | Not applicable | Static tracer; no motion claim |
| Accepted before frame/clip | A prior approved baseline exists | None | This candidate may become the first baseline only after Accept |
| Candidate after frame/clip | The visible result changed | normal-frame.png, debug-frame.png | Normal output plus explanatory diagnostic |
| State and metrics dump | Behavior/performance matters | state.json, configuration.json | Scenario, viewport, depth state, hashes, GL/GLSL configuration |
| Relevant logs | Warnings/errors matter | run.log | Commands, CTest results, capture hashes, GL diagnostics |

## Owner review

Review the normal and diagnostic evidence together.

- [ ] The packet answers the stated question rather than showcasing unrelated polish.
- [ ] Scenario, seed, tick, camera, viewport, and profile are comparable.
- [ ] The perspective, geometry, face separation, and silhouette are readable at the captured size or a nearest-neighbor zoom.
- [ ] The wireframe explains the submitted cube triangles without being mistaken for player-facing presentation.
- [ ] The zero-high-severity GL result and deterministic capture evidence are sufficient for this tracer.
- [ ] Known limitations are acceptable for Tracer 0.

### Verdict

Select exactly one:

- [ ] **Accept** - promote the named candidate artifacts to the accepted baseline for this scenario/profile.
- [ ] **Revise** - keep no baseline; the candidate needs the changes described below.
- [ ] **Reject** - keep no baseline and abandon or redesign this candidate direction.

**Owner observation and required follow-up:**

**Owner/date:**

## Baseline rule

Only an explicit **Accept** verdict can promote this candidate. Agent inspection and automated checks do not constitute owner acceptance, cross-GPU pixel identity, player understanding, or production-art approval.
"@
    $utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($ReviewPath, $review + [Environment]::NewLine, $utf8WithoutBom)
    [Console]::Out.WriteLine("visual_review_packet={0}" -f $ReviewPath)
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$artifactRoot = Join-Path $repoRoot ("artifacts\phase1\{0}" -f (Get-Date -Format "yyyy-MM-dd"))
$timestamp = Get-Date -Format "HHmmssfff"
$packetDirectory = Join-Path $artifactRoot ("windows-cube-smoke-{0}" -f $timestamp)
$localRoot = Join-Path $env:LOCALAPPDATA ("WideEye\phase1-context-{0}" -f $timestamp)
$localSource = Join-Path $localRoot "source"
$normalCapturePath = Join-Path $packetDirectory "normal-frame.png"
$repeatCapturePath = Join-Path $packetDirectory "repeat-frame.png"
$debugCapturePath = Join-Path $packetDirectory "debug-frame.png"
$manifestPath = Join-Path $packetDirectory "manifest.json"
$reviewPath = Join-Path $packetDirectory "review.md"
$configurationPath = Join-Path $packetDirectory "configuration.json"
$statePath = Join-Path $packetDirectory "state.json"
$sourceHashesPath = Join-Path $packetDirectory "source-hashes.json"
$startedAt = (Get-Date).ToUniversalTime()

New-Item -ItemType Directory -Force -Path $packetDirectory | Out-Null
$script:LogPath = Join-Path $packetDirectory "run.log"
Set-Content -LiteralPath $script:LogPath -Value "wide_eye_phase1_windows_cube_smoke" -Encoding UTF8

$scriptExitCode = 2
$runResult = "fail"
$failureStage = $null
$failureMessage = $null
$installationPath = $null
$cmake = $null
$ctest = $null
$sourceHashes = @()
$sourceState = Get-SourceState -RepositoryRoot $repoRoot
$platformState = Get-PlatformState

try {
    $script:CurrentStage = "visual-studio-discovery"
    $installationPath = Get-VisualStudioInstallation
    Import-VisualStudioEnvironment -InstallationPath $installationPath

    $cmakeBundled = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ctestBundled = Join-Path $installationPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
    $cmake = Resolve-RequiredTool -Name "cmake.exe" -BundledPath $cmakeBundled
    $ctest = Resolve-RequiredTool -Name "ctest.exe" -BundledPath $ctestBundled

    $script:CurrentStage = "source-snapshot"
    New-Item -ItemType Directory -Force -Path $localSource | Out-Null
    foreach ($directory in "cmake", "src", "tests") {
        Copy-Item -LiteralPath (Join-Path $repoRoot $directory) -Destination $localSource -Recurse
    }
    foreach ($file in ".clang-format", ".clang-tidy", "CMakeLists.txt", "CMakePresets.json") {
        Copy-Item -LiteralPath (Join-Path $repoRoot $file) -Destination $localSource
    }

    Write-Logged -Message "platform=native-windows"
    Write-Logged -Message ("visual_studio={0}" -f $installationPath)
    Write-Logged -Message ("local_root={0}" -f $localRoot)
    Write-Logged -Message ("artifact_packet={0}" -f $packetDirectory)
    Write-Logged -Message ("source_commit={0}" -f $sourceState.commit)
    Write-Logged -Message ("source_worktree_state={0}" -f $sourceState.worktree_state)

    $hashInputs = @(
        (Join-Path $repoRoot ".clang-format"),
        (Join-Path $repoRoot ".clang-tidy"),
        (Join-Path $repoRoot "CMakeLists.txt"),
        (Join-Path $repoRoot "CMakePresets.json"),
        $PSCommandPath
    )
    $hashInputs += Get-ChildItem -LiteralPath (Join-Path $repoRoot "cmake") -File -Recurse | Select-Object -ExpandProperty FullName
    $hashInputs += Get-ChildItem -LiteralPath (Join-Path $repoRoot "src") -File -Recurse | Select-Object -ExpandProperty FullName
    $hashInputs += Get-ChildItem -LiteralPath (Join-Path $repoRoot "tests") -File -Recurse | Select-Object -ExpandProperty FullName
    $sourceHashRecords = [System.Collections.Generic.List[object]]::new()
    foreach ($sourcePath in $hashInputs | Sort-Object -Unique) {
        $sourceHash = Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256
        $relativePath = $sourcePath.Substring($repoRoot.Length).TrimStart("\", "/")
        [void]$sourceHashRecords.Add([pscustomobject][ordered]@{
            path = $relativePath.Replace("\", "/")
            sha256 = $sourceHash.Hash.ToLowerInvariant()
        })
    }
    $sourceHashes = @($sourceHashRecords | ForEach-Object { $_ })

    Push-Location -LiteralPath $localSource
    try {
        Invoke-CheckedNative -Stage "configure" -Label "CMake configure" -FilePath $cmake -NativeArguments @("--preset", "dev")
        Invoke-CheckedNative -Stage "build" -Label "CMake build" -FilePath $cmake -NativeArguments @("--build", "--preset", "dev")
        Invoke-CheckedNative -Stage "ctest" -Label "CTest" -FilePath $ctest -NativeArguments @("--preset", "dev") -RecordState

        $executable = Join-Path $localSource "build\Windows\dev\wide_eye.exe"
        Invoke-CheckedNative -Stage "triangle-smoke" -Label "Native OpenGL triangle smoke" -FilePath $executable -NativeArguments @("--triangle-smoke") -RecordState
        Invoke-CheckedNative -Stage "voxel-cube-smoke" -Label "Native OpenGL voxel cube smoke" -FilePath $executable -NativeArguments @("--voxel-cube-smoke") -RecordState
        Invoke-CheckedNative `
            -Stage "capture-one" `
            -Label "Native OpenGL voxel cube capture one" `
            -FilePath $executable `
            -NativeArguments @("--voxel-cube-smoke", "--capture", $normalCapturePath) `
            -RecordState
        Invoke-CheckedNative `
            -Stage "capture-two" `
            -Label "Native OpenGL voxel cube capture two" `
            -FilePath $executable `
            -NativeArguments @("--voxel-cube-smoke", "--capture", $repeatCapturePath) `
            -RecordState
        Invoke-CheckedNative `
            -Stage "debug-capture" `
            -Label "Native OpenGL voxel cube wireframe capture" `
            -FilePath $executable `
            -NativeArguments @("--voxel-cube-debug-smoke", "--capture", $debugCapturePath) `
            -RecordState

        $script:CurrentStage = "capture-repeat-compare"
        if ($InjectCaptureMismatch.IsPresent) {
            Write-Logged -Message "failure_injection=capture_mismatch"
            Add-Content -LiteralPath $repeatCapturePath -Value ([byte]0) -Encoding Byte
        }
        $normalCaptureHash = Get-FileHash -LiteralPath $normalCapturePath -Algorithm SHA256
        $repeatCaptureHash = Get-FileHash -LiteralPath $repeatCapturePath -Algorithm SHA256
        $debugCaptureHash = Get-FileHash -LiteralPath $debugCapturePath -Algorithm SHA256
        Write-Logged -Message ("normal_capture_sha256={0}" -f $normalCaptureHash.Hash.ToLowerInvariant())
        Write-Logged -Message ("repeat_capture_sha256={0}" -f $repeatCaptureHash.Hash.ToLowerInvariant())
        Write-Logged -Message ("debug_capture_sha256={0}" -f $debugCaptureHash.Hash.ToLowerInvariant())
        $script:ObservedState["normal_capture_path"] = "normal-frame.png"
        $script:ObservedState["debug_capture_path"] = "debug-frame.png"
        $script:ObservedState["normal_capture_sha256"] = $normalCaptureHash.Hash.ToLowerInvariant()
        $script:ObservedState["repeat_capture_sha256"] = $repeatCaptureHash.Hash.ToLowerInvariant()
        $script:ObservedState["debug_capture_sha256"] = $debugCaptureHash.Hash.ToLowerInvariant()
        if ($normalCaptureHash.Hash -ne $repeatCaptureHash.Hash) {
            throw "Repeated direct capture hashes differ."
        }
        Remove-Item -LiteralPath $repeatCapturePath
    }
    finally {
        Pop-Location
    }

    Write-Logged -Message "result=pass"
    $runResult = "pass"
    $scriptExitCode = 0
}
catch {
    $failureStage = $script:CurrentStage
    $failureMessage = $_.Exception.Message
    foreach ($line in @("result=fail", ("failure_stage={0}" -f $failureStage), ("error={0}" -f $failureMessage))) {
        [Console]::Error.WriteLine($line)
        Add-Content -LiteralPath $script:LogPath -Value $line -Encoding UTF8
    }
}
finally {
    try {
        Write-ArtifactPacket `
            -PacketDirectory $packetDirectory `
            -ManifestPath $manifestPath `
            -ConfigurationPath $configurationPath `
            -StatePath $statePath `
            -SourceHashesPath $sourceHashesPath `
            -SourceState $sourceState `
            -PlatformState $platformState `
            -SourceHashes $sourceHashes `
            -Result $runResult `
            -FailureStage $failureStage `
            -FailureMessage $failureMessage `
            -CMakePath $cmake `
            -CTestPath $ctest `
            -LocalSource $localSource `
            -NormalCapturePath $normalCapturePath `
            -RepeatCapturePath $repeatCapturePath `
            -DebugCapturePath $debugCapturePath `
            -StartedAt $startedAt
        if ($runResult -eq "pass") {
            Write-VisualReviewPacket `
                -ReviewPath $reviewPath `
                -ManifestPath $manifestPath `
                -SourceState $sourceState `
                -PlatformState $platformState `
                -StartedAt $startedAt
        }
    }
    catch {
        [Console]::Error.WriteLine("artifact_manifest_result=fail")
        [Console]::Error.WriteLine("artifact_manifest_error={0}" -f $_.Exception.Message)
        $scriptExitCode = 3
    }
}

exit $scriptExitCode
