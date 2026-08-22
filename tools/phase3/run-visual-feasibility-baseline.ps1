[CmdletBinding()]
param(
    [ValidateRange(1, 16384)][int]$Width = 2560,
    [ValidateRange(1, 16384)][int]$Height = 1440,
    [ValidateRange(1, 1000)][int]$RefreshHz = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$scene = "visual-feasibility-five-sheep-v1"
$profile = "visual-feasibility-reference-high-v1"
$requiredRenderer = "NVIDIA GeForce RTX 4070 Ti"
$script:LogPath = ""
$script:Observed = [ordered]@{}
$script:Commands = [System.Collections.Generic.List[object]]::new()

function Write-JsonFile {
    param([string]$Path, [object]$Value, [int]$Depth = 16)
    [System.IO.File]::WriteAllText(
        $Path, ($Value | ConvertTo-Json -Depth $Depth) + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
}

function Write-Logged {
    param([AllowEmptyString()][string]$Message)
    [Console]::Out.WriteLine($Message)
    if (-not [string]::IsNullOrWhiteSpace($script:LogPath)) {
        Add-Content -LiteralPath $script:LogPath -Value $Message -Encoding UTF8
    }
}

function Invoke-Checked {
    param([string]$Stage, [string]$FilePath, [string[]]$Arguments,
          [string]$WorkingDirectory = "", [switch]$RecordState)
    $started = (Get-Date).ToUniversalTime()
    Write-Logged "command_stage=$Stage"
    Write-Logged ("command={0} {1}" -f $FilePath, ($Arguments -join " "))
    if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
        $output = & $FilePath @Arguments 2>&1
    }
    else {
        $quoted = @($Arguments | ForEach-Object { '"{0}"' -f $_.Replace('"', '\"') })
        $command = 'pushd "{0}" && "{1}" {2}' -f $WorkingDirectory, $FilePath, ($quoted -join " ")
        Push-Location -LiteralPath $env:SystemRoot
        try { $output = & $env:COMSPEC /d /s /c $command 2>&1 }
        finally { Pop-Location }
    }
    $output | ForEach-Object {
        $line = $_.ToString()
        Write-Logged $line
        if ($RecordState -and $line -match "^([A-Za-z][A-Za-z0-9_.-]*)=(.*)$") {
            $script:Observed[$Matches[1]] = $Matches[2]
        }
    }
    $exitCode = $LASTEXITCODE
    $finished = (Get-Date).ToUniversalTime()
    [void]$script:Commands.Add([pscustomobject][ordered]@{
        stage = $Stage; file = $FilePath; arguments = @($Arguments)
        started_at_utc = $started.ToString("o"); finished_at_utc = $finished.ToString("o")
        elapsed_ms = [math]::Round(($finished - $started).TotalMilliseconds, 3)
        exit_code = $exitCode; result = if ($exitCode -eq 0) { "pass" } else { "fail" }
    })
    if ($exitCode -ne 0) { throw "$Stage failed with exit code $exitCode." }
}

function Import-VisualStudioEnvironment {
    $root = ${env:ProgramFiles(x86)}
    $vswhere = Join-Path $root "Microsoft Visual Studio\Installer\vswhere.exe"
    $installation = & $vswhere -latest -products "*" `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installation)) {
        throw "A Visual Studio installation with the x64 C++ toolset was not found."
    }
    $vsDevCmd = Join-Path $installation.Trim() "Common7\Tools\VsDevCmd.bat"
    Push-Location -LiteralPath $env:SystemRoot
    try {
        $environment = & $env:COMSPEC /d /s /c `
            ('"{0}" -no_logo -arch=x64 -host_arch=x64 >nul && set' -f $vsDevCmd)
    }
    finally { Pop-Location }
    if ($LASTEXITCODE -ne 0) { throw "VsDevCmd.bat failed." }
    foreach ($line in $environment) {
        $separator = $line.IndexOf("=")
        if ($separator -gt 0) {
            [Environment]::SetEnvironmentVariable(
                $line.Substring(0, $separator), $line.Substring($separator + 1), "Process")
        }
    }
    return $installation.Trim()
}

function Add-Artifact {
    param($Records, [string]$Role, [string]$Path, [string]$MediaType,
          [string]$PacketDirectory)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return }
    $item = Get-Item -LiteralPath $Path
    [void]$Records.Add([pscustomobject][ordered]@{
        role = $Role
        path = $item.FullName.Substring($PacketDirectory.Length).TrimStart("\", "/").Replace("\", "/")
        media_type = $MediaType; bytes = $item.Length
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    })
}

function Capture-VisualTracer {
    param([string]$Stage, [string]$Camera, [int]$Tick, [string]$View,
          [string]$CapturePath, [string]$StatePath, [string]$Executable)
    Invoke-Checked $Stage $Executable @(
        "--visual-tracer-render-smoke", $scene, "--camera", $Camera,
        "--profile", $profile, "--width", "$Width", "--height", "$Height",
        "--refresh-hz", "$RefreshHz", "--tick", "$Tick", "--view", $View,
        "--capture", $CapturePath, "--state-dump", $StatePath) -RecordState
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$date = Get-Date -Format "yyyy-MM-dd"
$stamp = Get-Date -Format "HHmmssfff"
$packet = Join-Path $repoRoot "artifacts\phase3\$date\visual-feasibility-baseline-$stamp"
New-Item -ItemType Directory -Force -Path $packet | Out-Null
$script:LogPath = Join-Path $packet "run.log"
Set-Content -LiteralPath $script:LogPath -Value "wide_eye_visual_feasibility_baseline_v1" -Encoding UTF8
$started = (Get-Date).ToUniversalTime()
$result = "fail"
$failure = $null

$inventoryPath = Join-Path $packet "inventory.json"
$configurationPath = Join-Path $packet "configuration.json"
$measurementsPath = Join-Path $packet "measurements.json"
$sourceHashesPath = Join-Path $packet "source-hashes.json"
$rubricPath = Join-Path $packet "visual-rubric.md"
$reviewPath = Join-Path $packet "review.md"
$representative = Join-Path $packet "representative-normal.png"
$representativeRepeat = Join-Path $packet "representative-normal-repeat.png"
$representativeDebug = Join-Path $packet "representative-debug.png"
$holdout = Join-Path $packet "holdout-normal.png"
$holdoutDebug = Join-Path $packet "holdout-debug.png"
$stateReference = Join-Path $packet "state-tick-30.json"
$stateRepeat = Join-Path $packet "state-tick-30-repeat.json"
$stateDebug = Join-Path $packet "state-tick-30-debug.json"
$motionPaths = @(1, 30, 90 | ForEach-Object { Join-Path $packet "motion-tick-$_.png" })
$motionStatePaths = @(1, 30, 90 | ForEach-Object { Join-Path $packet "motion-state-tick-$_.json" })
$packagePath = Join-Path $packet "release-binaries.zip"

$os = Get-CimInstance Win32_OperatingSystem
$computer = Get-CimInstance Win32_ComputerSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$gpus = @(Get-CimInstance Win32_VideoController | ForEach-Object {
    [ordered]@{
        name = $_.Name; driver_version = $_.DriverVersion
        current_width = $_.CurrentHorizontalResolution
        current_height = $_.CurrentVerticalResolution
        current_refresh_hz = $_.CurrentRefreshRate
        video_mode = $_.VideoModeDescription
    }
})
$supportedModes = @()
try {
    $supportedModes = @(Get-CimInstance -Namespace root\wmi WmiMonitorListedSupportedSourceModes |
        ForEach-Object {
            $monitor = $_
            @($monitor.MonitorSourceModes | ForEach-Object {
                [ordered]@{
                    monitor = $monitor.InstanceName
                    width = $_.HorizontalActivePixels; height = $_.VerticalActivePixels
                    refresh_numerator = $_.VerticalRefreshRateNumerator
                    refresh_denominator = $_.VerticalRefreshRateDenominator
                }
            })
        })
}
catch { Write-Logged ("display_mode_query_warning={0}" -f $_.Exception.Message) }
$inventory = [ordered]@{
    schema_version = 1
    os = [ordered]@{ caption = $os.Caption; version = $os.Version; build = $os.BuildNumber }
    cpu = $cpu.Name; physical_memory_bytes = [uint64]$computer.TotalPhysicalMemory
    gpu_inventory = $gpus; supported_display_modes = $supportedModes
}
Write-JsonFile $inventoryPath $inventory

try {
    $modeSupported = @($supportedModes | Where-Object {
        $_.width -eq $Width -and $_.height -eq $Height -and $_.refresh_denominator -ne 0 -and
        ($_.refresh_numerator / $_.refresh_denominator) -ge $RefreshHz
    }).Count -gt 0
    if (-not $modeSupported) {
        throw "The requested ${Width}x${Height}@${RefreshHz} mode was not reported by the display."
    }

    $installation = Import-VisualStudioEnvironment
    $cmake = Join-Path $installation "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ctest = Join-Path $installation "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
    Invoke-Checked "configure-release" $cmake @("--preset", "release") $repoRoot
    Invoke-Checked "build-release" $cmake @("--build", "--preset", "release") $repoRoot
    Invoke-Checked "ctest-release" $ctest @("--preset", "release") $repoRoot -RecordState
    $executable = Join-Path $repoRoot "build\Windows\release\wide_eye.exe"
    Invoke-Checked "visual-tracer-configuration" $executable @(
        "--visual-tracer-configuration", $scene) -RecordState

    Capture-VisualTracer "representative-normal" "representative" 30 "normal" `
        $representative $stateReference $executable
    if ($script:Observed["gl_renderer"] -notlike "*$requiredRenderer*") {
        throw "Active OpenGL renderer '$($script:Observed['gl_renderer'])' is not $requiredRenderer."
    }
    if ($script:Observed["actual_gl"] -ne "4.6") {
        throw "The active OpenGL context is '$($script:Observed['actual_gl'])', not 4.6."
    }
    Capture-VisualTracer "representative-normal-repeat" "representative" 30 "normal" `
        $representativeRepeat $stateRepeat $executable
    Capture-VisualTracer "representative-debug" "representative" 30 "debug" `
        $representativeDebug $stateDebug $executable
    Capture-VisualTracer "holdout-normal" "holdout" 30 "normal" $holdout `
        (Join-Path $packet "holdout-state-tick-30.json") $executable
    Capture-VisualTracer "holdout-debug" "holdout" 30 "debug" $holdoutDebug `
        (Join-Path $packet "holdout-state-tick-30-debug.json") $executable
    for ($index = 0; $index -lt 3; ++$index) {
        $tick = @(1, 30, 90)[$index]
        Capture-VisualTracer "motion-tick-$tick" "representative" $tick "normal" `
            $motionPaths[$index] $motionStatePaths[$index] $executable
    }
    Invoke-Checked "visual-tracer-performance" $executable @(
        "--visual-tracer-performance-smoke", $scene, "--profile", $profile,
        "--width", "$Width", "--height", "$Height", "--refresh-hz", "$RefreshHz") -RecordState

    if ((Get-FileHash $representative -Algorithm SHA256).Hash -ne
        (Get-FileHash $representativeRepeat -Algorithm SHA256).Hash) {
        throw "Repeated representative captures differ."
    }
    if ((Get-FileHash $stateReference -Algorithm SHA256).Hash -ne
        (Get-FileHash $stateRepeat -Algorithm SHA256).Hash -or
        (Get-FileHash $stateReference -Algorithm SHA256).Hash -ne
        (Get-FileHash $stateDebug -Algorithm SHA256).Hash) {
        throw "Same-state gameplay dumps differ across normal/debug/repeat captures."
    }
    if ($script:Observed["gl_debug_high_severity_messages"] -ne "0" -or
        $script:Observed["within_performance_budget"] -ne "yes") {
        throw "Graphics diagnostics or the reference performance budget failed."
    }

    $sdl = Join-Path (Split-Path -Parent $executable) "SDL3.dll"
    Compress-Archive -LiteralPath @($executable, $sdl) -DestinationPath $packagePath -Force
    $packageBytes = (Get-Item $packagePath).Length
    if ($packageBytes -gt 67108864) { throw "Compressed Release binaries exceed 64 MiB." }
    $firstCapture = $script:Commands | Where-Object stage -eq "representative-normal" |
        Select-Object -First 1
    $script:Observed["startup_to_first_capture_ms"] = $firstCapture.elapsed_ms
    $script:Observed["compressed_package_bytes"] = $packageBytes
    $script:Observed["same_state_capture_repeat"] = "yes"
    $script:Observed["same_state_dump_repeat"] = "yes"
    if ($firstCapture.elapsed_ms -gt 3000) { throw "Startup-to-first-capture exceeded 3 seconds." }
    $result = "pass"
}
catch {
    $failure = [ordered]@{ message = $_.Exception.Message }
    Write-Logged "failure_stage=visual_feasibility_baseline_runner"
    Write-Logged ("failure_message={0}" -f $_.Exception.Message)
}

$configuration = [ordered]@{
    schema_version = 1; scene = $scene; profile = $profile
    gameplay_scenario = $script:Observed["visual_tracer_gameplay_scenario"]
    scenario_version = $script:Observed["visual_tracer_gameplay_scenario_version"]
    seed = $script:Observed["visual_tracer_seed"]
    route = $script:Observed["visual_tracer_route"]
    route_version = $script:Observed["visual_tracer_route_version"]
    reference_tick = 30; motion_ticks = @(1, 30, 90)
    representative_camera = "representative"; holdout_camera = "holdout"
    viewport = [ordered]@{ width = $Width; height = $Height; refresh_hz = $RefreshHz }
    comparison_role = "unchanged pre-visual-implementation evidence"
}
Write-JsonFile $configurationPath $configuration
Write-JsonFile $measurementsPath ([pscustomobject]$script:Observed)

$hashInputs = @((Join-Path $repoRoot "CMakeLists.txt"), (Join-Path $repoRoot "CMakePresets.json"),
    $PSCommandPath)
foreach ($directory in "cmake", "src", "tests", "third_party") {
    $hashInputs += Get-ChildItem -LiteralPath (Join-Path $repoRoot $directory) -File -Recurse |
        Select-Object -ExpandProperty FullName
}
$sourceHashes = @($hashInputs | Sort-Object -Unique | ForEach-Object {
    [ordered]@{
        path = $_.Substring($repoRoot.Length).TrimStart("\", "/").Replace("\", "/")
        sha256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})
Write-JsonFile $sourceHashesPath ([ordered]@{ schema_version = 1; files = $sourceHashes })

$rubric = @"
# Visual-feasibility owner rubric

The unchanged frame is comparison evidence, not a scored candidate. Confirm only that the
selected representative route and untouched holdout expose the intended questions before tuning.

| Review question | Unchanged baseline note | Candidate question for later phases |
| --- | --- | --- |
| Composition and scale | Record what the bounded paddock currently exposes. | Does the scene read as a broader layered countryside in both views? |
| Geometry and vegetation | Do not fail absent features in this packet. | Is the voxel-informed detail fine enough, dense enough, and still readable? |
| Palette, materials, light, and shadows | Record ambiguity or instability. | Are values deliberate, shadows stable, and subjects separated from terrain? |
| Atmosphere and focal readability | Record current fog/edge behavior. | Does depth/softness help without blurring dog, sheep, route, gates, or terrain edges? |
| Animals and motion | Five authoritative sheep only. | Are silhouette, facing, gait, and stable identity readable along the route? |
| Representative/holdout agreement | Confirm both cameras ask useful questions. | Does improvement survive the untouched holdout without frame-by-frame tuning? |

- [ ] The representative camera and route ask the intended close-readability question.
- [ ] The holdout camera asks the intended landscape-depth/generalization question.

**Owner observation:**

**Owner/date:**
"@
[System.IO.File]::WriteAllText($rubricPath, $rubric + [Environment]::NewLine,
    [System.Text.UTF8Encoding]::new($false))

$git = Get-Command git.exe -ErrorAction SilentlyContinue
$commit = $null
$status = @()
if ($null -ne $git) {
    $safeRoot = $repoRoot.Replace("\", "/")
    $commit = (& $git.Source -c "safe.directory=$safeRoot" -C $repoRoot rev-parse HEAD).Trim()
    $status = @(& $git.Source -c "safe.directory=$safeRoot" -C $repoRoot status --short)
}

$review = @"
# Phase 0 visual-feasibility unchanged baseline

This packet records the unchanged renderer and five-sheep gameplay state. It is not a promoted
visual golden and does not accept a candidate look, 25/100 sheep, or a shipping minimum.

Automated result: **$result**

- Active OpenGL renderer: $($script:Observed["gl_renderer"])
- OpenGL version: $($script:Observed["gl_version"])
- Viewport: ${Width}x${Height}@${RefreshHz}
- Same-state capture repeat: $($script:Observed["same_state_capture_repeat"])
- Same-state state repeat: $($script:Observed["same_state_dump_repeat"])
- High-severity OpenGL messages: $($script:Observed["gl_debug_high_severity_messages"])
- Reference budget: $($script:Observed["within_performance_budget"])

## Owner camera/rubric confirmation

- [ ] **Confirm** - both views ask the intended Phase 0 visual question.
- [ ] **Revise** - change a named camera/route/viewport before visual tuning.

**Owner observation and required follow-up:**

**Owner/date:**
"@
[System.IO.File]::WriteAllText($reviewPath, $review + [Environment]::NewLine,
    [System.Text.UTF8Encoding]::new($false))

$records = [System.Collections.Generic.List[object]]::new()
foreach ($entry in @(
    @("log", $script:LogPath, "text/plain"), @("inventory", $inventoryPath, "application/json"),
    @("configuration", $configurationPath, "application/json"),
    @("measurements", $measurementsPath, "application/json"),
    @("source_hashes", $sourceHashesPath, "application/json"),
    @("visual_rubric", $rubricPath, "text/markdown"), @("review", $reviewPath, "text/markdown"),
    @("representative_normal", $representative, "image/png"),
    @("representative_normal_repeat", $representativeRepeat, "image/png"),
    @("representative_debug", $representativeDebug, "image/png"),
    @("holdout_normal", $holdout, "image/png"), @("holdout_debug", $holdoutDebug, "image/png"),
    @("state_reference", $stateReference, "application/json"),
    @("release_binaries", $packagePath, "application/zip"))) {
    Add-Artifact $records $entry[0] $entry[1] $entry[2] $packet
}
for ($index = 0; $index -lt 3; ++$index) {
    Add-Artifact $records "motion_tick_$(@(1, 30, 90)[$index])" $motionPaths[$index] `
        "image/png" $packet
    Add-Artifact $records "motion_state_tick_$(@(1, 30, 90)[$index])" `
        $motionStatePaths[$index] "application/json" $packet
}
$manifest = [ordered]@{
    schema = "wide-eye.artifact-manifest"; schema_version = 1
    packet_version = "visual-feasibility-baseline-v1"; result = $result
    started_at_utc = $started.ToString("o")
    finished_at_utc = (Get-Date).ToUniversalTime().ToString("o")
    source = [ordered]@{ commit = $commit; worktree_state = if ($status.Count) { "dirty" } else { "clean" }; status = $status }
    platform = "inventory.json"; configuration = "configuration.json"
    measurements = "measurements.json"; source_hashes = "source-hashes.json"
    review_document = "review.md"; rubric = "visual-rubric.md"
    reproduction_command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Width $Width -Height $Height -RefreshHz $RefreshHz"
    workspace_local = $true; commands = @($script:Commands); failure = $failure
    artifacts = @($records)
}
$manifestPath = Join-Path $packet "manifest.json"
Write-JsonFile $manifestPath $manifest

if ($result -eq "pass") {
    $validator = Join-Path $repoRoot "tests\assert-visual-feasibility-baseline-manifest.cmake"
    $validationOutput = & $cmake "-DMANIFEST=$manifestPath" -P $validator 2>&1
    $validationOutput | ForEach-Object { [Console]::Out.WriteLine($_.ToString()) }
    if ($LASTEXITCODE -ne 0) {
        $result = "fail"
        $manifest.result = "fail"
        $manifest.failure = [ordered]@{ message = "Artifact-manifest validation failed." }
        Write-JsonFile $manifestPath $manifest
    }
}

[Console]::Out.WriteLine("artifact_manifest=$manifestPath")
[Console]::Out.WriteLine("visual_review_packet=$reviewPath")
[Console]::Out.WriteLine("visual_feasibility_baseline_result=$result")
if ($result -ne "pass") { exit 1 }
