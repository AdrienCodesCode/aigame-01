[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:LogPath = ""
$script:ObservedState = [ordered]@{}
$script:Commands = [System.Collections.Generic.List[object]]::new()

function Write-JsonFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][object]$Value,
        [int]$Depth = 12
    )
    $json = $Value | ConvertTo-Json -Depth $Depth
    $encoding = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $encoding)
}

function Write-Logged {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Message)
    [Console]::Out.WriteLine($Message)
    if (-not [string]::IsNullOrWhiteSpace($script:LogPath)) {
        Add-Content -LiteralPath $script:LogPath -Value $Message -Encoding UTF8
    }
}

function Add-ObservedLines {
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

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [string]$WorkingDirectory = "",
        [switch]$RecordState
    )
    $started = (Get-Date).ToUniversalTime()
    $lines = [System.Collections.Generic.List[string]]::new()
    Write-Logged -Message ("command_stage={0}" -f $Stage)
    $displayCommand = "{0} {1}" -f $FilePath, ($Arguments -join " ")
    Write-Logged -Message ("command={0}" -f $displayCommand)
    if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
        $commandOutput = & $FilePath @Arguments 2>&1
    }
    else {
        $quotedArguments = @($Arguments | ForEach-Object { '"{0}"' -f $_.Replace('"', '\"') })
        $cmdCommand = 'pushd "{0}" && "{1}" {2}' -f $WorkingDirectory, $FilePath, ($quotedArguments -join " ")
        Push-Location -LiteralPath $env:SystemRoot
        try {
            $commandOutput = & $env:COMSPEC /d /s /c $cmdCommand 2>&1
        }
        finally {
            Pop-Location
        }
    }
    $commandOutput | ForEach-Object {
        $line = $_.ToString()
        [void]$lines.Add($line)
        Write-Logged -Message $line
    }
    $exitCode = $LASTEXITCODE
    $finished = (Get-Date).ToUniversalTime()
    [void]$script:Commands.Add([pscustomobject][ordered]@{
        stage = $Stage
        file = $FilePath
        arguments = @($Arguments)
        started_at_utc = $started.ToString("o")
        finished_at_utc = $finished.ToString("o")
        exit_code = $exitCode
        result = if ($exitCode -eq 0) { "pass" } else { "fail" }
    })
    if ($RecordState) {
        Add-ObservedLines -Lines ([string[]]$lines.ToArray())
    }
    if ($exitCode -ne 0) {
        throw "$Stage failed with exit code $exitCode."
    }
}

function Get-VisualStudioInstallation {
    $installerRoot = ${env:ProgramFiles(x86)}
    if ([string]::IsNullOrWhiteSpace($installerRoot)) {
        throw "ProgramFiles(x86) is unavailable."
    }
    $vswhere = Join-Path $installerRoot "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "Visual Studio Build Tools were not found."
    }
    $installation = & $vswhere -latest -products "*" `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installation)) {
        throw "A Visual Studio installation with the x64 C++ toolset was not found."
    }
    return $installation.Trim()
}

function Import-VisualStudioEnvironment {
    param([Parameter(Mandatory = $true)][string]$InstallationPath)
    $vsDevCmd = Join-Path $InstallationPath "Common7\Tools\VsDevCmd.bat"
    $command = '"{0}" -no_logo -arch=x64 -host_arch=x64 >nul && set' -f $vsDevCmd
    Push-Location -LiteralPath $env:SystemRoot
    try {
        $environmentLines = & $env:COMSPEC /d /s /c $command
        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
    if ($exitCode -ne 0) {
        throw "VsDevCmd.bat failed with exit code $exitCode."
    }
    foreach ($line in $environmentLines) {
        $separator = $line.IndexOf("=")
        if ($separator -gt 0) {
            [Environment]::SetEnvironmentVariable(
                $line.Substring(0, $separator), $line.Substring($separator + 1), "Process")
        }
    }
}

function Get-SourceState {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)
    $state = [ordered]@{ commit = $null; worktree_state = "unknown"; status = @() }
    $git = Get-Command git.exe -ErrorAction SilentlyContinue
    if ($null -eq $git) {
        return [pscustomobject]$state
    }
    $safeRoot = $RepositoryRoot.Replace("\", "/")
    $commit = & $git.Source -c "safe.directory=$safeRoot" -C $RepositoryRoot rev-parse HEAD
    if ($LASTEXITCODE -ne 0) {
        return [pscustomobject]$state
    }
    [string[]]$status = @(& $git.Source -c "safe.directory=$safeRoot" -C $RepositoryRoot status --short)
    $state.commit = $commit.Trim()
    $state.status = $status
    $state.worktree_state = if ($status.Count -eq 0) { "clean" } else { "dirty" }
    return [pscustomobject]$state
}

function Get-PlatformState {
    $os = Get-CimInstance Win32_OperatingSystem
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $gpus = @(Get-CimInstance Win32_VideoController | ForEach-Object {
        [pscustomobject][ordered]@{ name = $_.Name; driver_version = $_.DriverVersion }
    })
    return [pscustomobject][ordered]@{
        name = "native-windows"
        os = $os.Caption + " " + $os.Version + " build " + $os.BuildNumber
        architecture = $env:PROCESSOR_ARCHITECTURE
        cpu = $cpu.Name
        gpus = $gpus
    }
}

function Get-StringSha256 {
    param([Parameter(Mandatory = $true)][string]$Value)
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
        [Parameter(Mandatory = $true)]$Records,
        [Parameter(Mandatory = $true)][string]$Role,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$MediaType,
        [Parameter(Mandatory = $true)][string]$PacketDirectory
    )
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return
    }
    $item = Get-Item -LiteralPath $Path
    $hash = Get-FileHash -LiteralPath $Path -Algorithm SHA256
    [void]$Records.Add([pscustomobject][ordered]@{
        role = $Role
        path = $item.FullName.Substring($PacketDirectory.Length).TrimStart("\", "/").Replace("\", "/")
        media_type = $MediaType
        bytes = $item.Length
        sha256 = $hash.Hash.ToLowerInvariant()
    })
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$date = Get-Date -Format "yyyy-MM-dd"
$timestamp = Get-Date -Format "HHmmssfff"
$packetDirectory = Join-Path $repoRoot ("artifacts\phase2\{0}\tracer1-review-windows-{1}" -f $date, $timestamp)
$normalCapture = Join-Path $packetDirectory "normal.png"
$normalRepeatCapture = Join-Path $packetDirectory "normal-repeat.png"
$chunkBoundsCapture = Join-Path $packetDirectory "chunk-bounds.png"
$faceNormalsCapture = Join-Path $packetDirectory "face-normals.png"
$wireframeCapture = Join-Path $packetDirectory "wireframe.png"
$meshStatisticsCapture = Join-Path $packetDirectory "mesh-statistics.png"
$dogCapture = Join-Path $packetDirectory "dog-placeholder.png"
$statePath = Join-Path $packetDirectory "state.json"
$configurationPath = Join-Path $packetDirectory "configuration.json"
$sourceHashesPath = Join-Path $packetDirectory "source-hashes.json"
$manifestPath = Join-Path $packetDirectory "manifest.json"
$reviewPath = Join-Path $packetDirectory "review.md"
$startedAt = (Get-Date).ToUniversalTime()

New-Item -ItemType Directory -Force -Path $packetDirectory | Out-Null
$script:LogPath = Join-Path $packetDirectory "run.log"
Set-Content -LiteralPath $script:LogPath -Value "wide_eye_tracer1_workspace_local_review" -Encoding UTF8

$sourceState = Get-SourceState -RepositoryRoot $repoRoot
$platformState = Get-PlatformState
$sourceHashes = @()
$result = "fail"
$failure = $null

try {
    $installation = Get-VisualStudioInstallation
    Import-VisualStudioEnvironment -InstallationPath $installation
    $cmake = Join-Path $installation "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ctest = Join-Path $installation "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
    if (-not (Test-Path -LiteralPath $cmake -PathType Leaf) -or
        -not (Test-Path -LiteralPath $ctest -PathType Leaf)) {
        throw "Visual Studio's CMake/CTest tools were not found."
    }

    Write-Logged -Message "evidence_scope=workspace_local"
    Write-Logged -Message ("repository_root={0}" -f $repoRoot)
    Write-Logged -Message ("artifact_packet={0}" -f $packetDirectory)

    $hashInputs = @(
        (Join-Path $repoRoot ".clang-format"),
        (Join-Path $repoRoot ".clang-tidy"),
        (Join-Path $repoRoot "CMakeLists.txt"),
        (Join-Path $repoRoot "CMakePresets.json"),
        $PSCommandPath
    )
    foreach ($directory in "cmake", "src", "tests", "third_party") {
        $hashInputs += Get-ChildItem -LiteralPath (Join-Path $repoRoot $directory) -File -Recurse |
            Select-Object -ExpandProperty FullName
    }
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

    Invoke-Checked -Stage "configure-release" -FilePath $cmake `
        -Arguments @("--preset", "release") -WorkingDirectory $repoRoot
    Invoke-Checked -Stage "build-release" -FilePath $cmake `
        -Arguments @("--build", "--preset", "release") -WorkingDirectory $repoRoot
    Invoke-Checked -Stage "ctest-release" -FilePath $ctest `
        -Arguments @("--preset", "release") -WorkingDirectory $repoRoot -RecordState
    $executable = Join-Path $repoRoot "build\Windows\release\wide_eye.exe"
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "The release executable was not produced at $executable."
    }

    Invoke-Checked -Stage "normal-capture" -FilePath $executable `
        -Arguments @("--paddock-smoke", "--capture", $normalCapture) -RecordState
    Invoke-Checked -Stage "normal-repeat-capture" -FilePath $executable `
        -Arguments @("--paddock-smoke", "--capture", $normalRepeatCapture) -RecordState
    Invoke-Checked -Stage "chunk-bounds-capture" -FilePath $executable `
        -Arguments @("--paddock-chunk-bounds-smoke", "--capture", $chunkBoundsCapture) -RecordState
    Invoke-Checked -Stage "face-normals-capture" -FilePath $executable `
        -Arguments @("--paddock-face-normals-smoke", "--capture", $faceNormalsCapture) -RecordState
    Invoke-Checked -Stage "wireframe-capture" -FilePath $executable `
        -Arguments @("--paddock-wireframe-smoke", "--capture", $wireframeCapture) -RecordState
    Invoke-Checked -Stage "mesh-statistics-capture" -FilePath $executable `
        -Arguments @("--paddock-mesh-statistics-smoke", "--capture", $meshStatisticsCapture) -RecordState
    Invoke-Checked -Stage "dog-placeholder-capture" -FilePath $executable `
        -Arguments @("--dog-render-smoke", "paddock-start", "--capture", $dogCapture) -RecordState
    Invoke-Checked -Stage "release-performance" -FilePath $executable `
        -Arguments @("--paddock-performance-smoke") -RecordState

    $normalHash = (Get-FileHash -LiteralPath $normalCapture -Algorithm SHA256).Hash
    $repeatHash = (Get-FileHash -LiteralPath $normalRepeatCapture -Algorithm SHA256).Hash
    if ($normalHash -ne $repeatHash) {
        throw "Repeated normal captures were not byte-identical."
    }
    $script:ObservedState["normal_repeat_sha256"] = $normalHash.ToLowerInvariant()
    $script:ObservedState["normal_repeat_matches"] = "yes"
    $result = "pass"
}
catch {
    $failure = [ordered]@{ message = $_.Exception.Message }
    Write-Logged -Message ("failure_stage=tracer1_review_runner")
    Write-Logged -Message ("failure_message={0}" -f $_.Exception.Message)
}

$configuration = [ordered]@{
    schema_version = 1
    build_preset = "release"
    viewport = [ordered]@{ capture_width = 960; capture_height = 540; performance_width = 1920; performance_height = 1080 }
    camera = "tracer1_fixed_blockout"
    graphics_profile = "development-low-proxy"
    normal_view = "normal"
    debug_views = @("chunk_bounds", "face_normals", "wireframe", "mesh_statistics")
    performance = [ordered]@{
        scenario = "handcrafted_paddock_static_v1"
        warmup_frames = 120
        sample_frames = 600
        timing_mode = "serialized_gpu_query_and_swap"
    }
}
Write-JsonFile -Path $configurationPath -Value $configuration
Write-JsonFile -Path $statePath -Value ([pscustomobject]$script:ObservedState)
$sourceInventory = [ordered]@{
    schema_version = 1
    aggregate_sha256 = Get-StringSha256 -Value (($sourceHashes | ForEach-Object { "{0}={1}" -f $_.path, $_.sha256 }) -join "`n")
    files = @($sourceHashes)
}
Write-JsonFile -Path $sourceHashesPath -Value $sourceInventory

$records = [System.Collections.Generic.List[object]]::new()
Add-ArtifactRecord -Records $records -Role "log" -Path $script:LogPath -MediaType "text/plain" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "configuration" -Path $configurationPath -MediaType "application/json" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "state" -Path $statePath -MediaType "application/json" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "source_hashes" -Path $sourceHashesPath -MediaType "application/json" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "normal_capture" -Path $normalCapture -MediaType "image/png" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "normal_repeat_capture" -Path $normalRepeatCapture -MediaType "image/png" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "chunk_bounds_capture" -Path $chunkBoundsCapture -MediaType "image/png" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "face_normals_capture" -Path $faceNormalsCapture -MediaType "image/png" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "wireframe_capture" -Path $wireframeCapture -MediaType "image/png" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "mesh_statistics_capture" -Path $meshStatisticsCapture -MediaType "image/png" -PacketDirectory $packetDirectory
Add-ArtifactRecord -Records $records -Role "dog_placeholder_capture" -Path $dogCapture -MediaType "image/png" -PacketDirectory $packetDirectory

$manifest = [ordered]@{
    schema = "wide-eye.artifact-manifest"
    schema_version = 1
    packet_version = "tracer1-same-state-review-v1"
    result = $result
    started_at_utc = $startedAt.ToString("o")
    finished_at_utc = (Get-Date).ToUniversalTime().ToString("o")
    source = $sourceState
    platform = $platformState
    build = [ordered]@{ configure_preset = "release"; build_preset = "release"; test_preset = "release" }
    scenario = [ordered]@{
        name = "handcrafted_paddock_static"
        version = 1
        camera = "tracer1_fixed_blockout"
        viewport = [ordered]@{ width = 960; height = 540 }
        performance_viewport = [ordered]@{ width = 1920; height = 1080 }
        graphics_profile = "development-low-proxy"
    }
    reproduction_command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    workspace_local = $true
    review_document = "review.md"
    commands = @($script:Commands | ForEach-Object { $_ })
    failure = $failure
    artifacts = @($records | ForEach-Object { $_ })
}
Write-JsonFile -Path $manifestPath -Value $manifest
$manifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()

$review = @"
# Tracer 1 same-state and dog-placeholder candidate review

This is a candidate review packet. The owner verdict is intentionally blank, and the accepted Tracer 1 blockout baseline is unchanged.

## Review question

- **Coherent outcome:** Verify the normal and four diagnostic paddock views from one fixed camera, record a release frame-time/RSS baseline, and make the first kinematic dog placeholder visible.
- **Question for the owner:** Are the same-camera diagnostics and placeholder dog readable enough to close Tracer 1?
- **What intentionally changed:** The rendered upright-cylinder dog now uses free mouse orbit, camera-relative movement, movement-driven facing, a bounded planar motor, and interpolated dog/camera presentation. Analytic collision, free-debug state, named keyboard/mouse/gamepad input, restart, deterministic scenarios, and measurement instrumentation remain separate.
- **What must remain invariant:** The accepted blockout packet is not replaced; normal/debug paddock captures use the same fixed camera, 960x540 viewport, geometry, and release build; high-severity GL messages remain zero.
- **Known limitations or unverified claims:** The dog is an engineering placeholder without animation. The performance host is an Intel UHD 630 laptop and is a low-target proxy, not the provisional Iris Xe reference device. The serialized measurement deliberately blocks for each GPU query and includes swap overhead. The keyboard/mouse control baseline was owner-accepted on 2026-08-16, but tuning is provisional. Native Linux graphics and a physical controller remain unverified.

## Reproduction metadata

| Field | Value |
| --- | --- |
| Date/time and timezone | $(Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz") |
| Commit and dirty-worktree state | $($sourceState.commit); $($sourceState.worktree_state) |
| Configure/build preset | release / release |
| OS and architecture | $($platformState.os); $($platformState.architecture) |
| CPU | $($platformState.cpu) |
| Active GPU/driver | $($script:ObservedState["gl_renderer"]); inventory retained in manifest.json |
| OpenGL and GLSL | $($script:ObservedState["gl_version"]); $($script:ObservedState["glsl_version"]) |
| Scenario and version | handcrafted_paddock_static, version 1 |
| Camera and viewport | tracer1_fixed_blockout; 960x540 captures; 1920x1080 measurement |
| Graphics profile | development-low-proxy |
| Hashed build inputs | source-hashes.json / $($sourceInventory.aggregate_sha256) |
| Exact reproduction command | powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$PSCommandPath" |
| Artifact manifest | manifest.json / $manifestHash |

## Automated evidence

| Check or budget | Result | Evidence |
| --- | --- | --- |
| Release CTest suite | $result | run.log |
| Same-camera normal/debug captures | Normal repeated byte-identically; four named debug paths captured when packet result is pass | normal.png, normal-repeat.png, chunk-bounds.png, face-normals.png, wireframe.png, mesh-statistics.png |
| GL diagnostics | High-severity count: $($script:ObservedState["gl_debug_high_severity_messages"]) | run.log, state.json |
| Release frame time | synchronized p95 $($script:ObservedState["synchronized_frame_p95_ns"]) ns; p99 $($script:ObservedState["synchronized_frame_p99_ns"]) ns | state.json, run.log |
| GPU render time | p95 $($script:ObservedState["gpu_render_p95_ns"]) ns; p99 $($script:ObservedState["gpu_render_p99_ns"]) ns | state.json, run.log |
| Process memory | current RSS $($script:ObservedState["process_rss_bytes"]) bytes; peak RSS $($script:ObservedState["process_peak_rss_bytes"]) bytes | state.json, run.log |
| Provisional low budget comparison | $($script:ObservedState["within_provisional_low_budget"]) on this proxy host | state.json, run.log |

## Artifacts to inspect

- normal.png together with all four same-camera diagnostics.
- dog-placeholder.png for silhouette, ground contact, and gameplay-camera composition.
- state.json and run.log for measurement method, percentiles, memory, and GL diagnostics.

## Owner review

- [ ] Normal and debug captures are genuinely comparable and each diagnostic explains its intended geometry.
- [ ] The dog is visibly grounded, its facing marker is readable, and the gameplay camera is a usable starting point.
- [x] The owner accepted the camera-relative keyboard/mouse baseline on 2026-08-16 and deferred refinement.
- [ ] A physical controller produces the same named actions with acceptable dead-zone behavior.
- [ ] The measured proxy result and known limitations are acceptable for Tracer 1.

### Verdict

- [ ] **Accept**
- [ ] **Revise**
- [ ] **Reject**

**Owner observation and required follow-up:**

**Owner/date:**
"@
$encoding = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($reviewPath, $review + [Environment]::NewLine, $encoding)

[Console]::Out.WriteLine("artifact_manifest={0}" -f $manifestPath)
[Console]::Out.WriteLine("visual_review_packet={0}" -f $reviewPath)
[Console]::Out.WriteLine("tracer1_review_result={0}" -f $result)
if ($result -ne "pass") {
    exit 1
}
