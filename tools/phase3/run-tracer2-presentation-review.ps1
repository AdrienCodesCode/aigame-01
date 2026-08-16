[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$script:LogPath = ""
$script:Observed = [ordered]@{}
$script:Commands = [System.Collections.Generic.List[object]]::new()

function Write-JsonFile {
    param([string]$Path, [object]$Value, [int]$Depth = 14)
    $json = $Value | ConvertTo-Json -Depth $Depth
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine,
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
    $lines = [System.Collections.Generic.List[string]]::new()
    $output | ForEach-Object {
        $line = $_.ToString()
        [void]$lines.Add($line)
        Write-Logged $line
        if ($RecordState -and $line -match "^([A-Za-z][A-Za-z0-9_.-]*)=(.*)$") {
            $script:Observed[$Matches[1]] = $Matches[2]
        }
    }
    $exitCode = $LASTEXITCODE
    [void]$script:Commands.Add([pscustomobject][ordered]@{
        stage = $Stage; file = $FilePath; arguments = @($Arguments)
        started_at_utc = $started.ToString("o")
        finished_at_utc = (Get-Date).ToUniversalTime().ToString("o")
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
    try { $environment = & $env:COMSPEC /d /s /c ('"{0}" -no_logo -arch=x64 -host_arch=x64 >nul && set' -f $vsDevCmd) }
    finally { Pop-Location }
    if ($LASTEXITCODE -ne 0) { throw "VsDevCmd.bat failed." }
    foreach ($line in $environment) {
        $separator = $line.IndexOf("=")
        if ($separator -gt 0) {
            [Environment]::SetEnvironmentVariable($line.Substring(0, $separator),
                $line.Substring($separator + 1), "Process")
        }
    }
    return $installation.Trim()
}

function Add-Artifact {
    param($Records, [string]$Role, [string]$Path, [string]$MediaType, [string]$PacketDirectory)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return }
    $item = Get-Item -LiteralPath $Path
    [void]$Records.Add([pscustomobject][ordered]@{
        role = $Role
        path = $item.FullName.Substring($PacketDirectory.Length).TrimStart("\", "/").Replace("\", "/")
        media_type = $MediaType
        bytes = $item.Length
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    })
}

function New-ContactSheet {
    param([string[]]$Paths, [string]$OutputPath)
    Add-Type -AssemblyName System.Drawing
    $sheet = [System.Drawing.Bitmap]::new(1920, 360)
    $graphics = [System.Drawing.Graphics]::FromImage($sheet)
    try {
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        for ($index = 0; $index -lt $Paths.Count; ++$index) {
            $image = [System.Drawing.Image]::FromFile($Paths[$index])
            try { $graphics.DrawImage($image, $index * 640, 0, 640, 360) }
            finally { $image.Dispose() }
        }
        $sheet.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally { $graphics.Dispose(); $sheet.Dispose() }
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$date = Get-Date -Format "yyyy-MM-dd"
$stamp = Get-Date -Format "HHmmssfff"
$packet = Join-Path $repoRoot "artifacts\phase3\$date\tracer2-presentation-windows-$stamp"
New-Item -ItemType Directory -Force -Path $packet | Out-Null
$script:LogPath = Join-Path $packet "run.log"
Set-Content -LiteralPath $script:LogPath -Value "wide_eye_tracer2_presentation_review" -Encoding UTF8
$started = (Get-Date).ToUniversalTime()
$result = "fail"
$failure = $null

$normal = Join-Path $packet "same-state-normal.png"
$repeat = Join-Path $packet "same-state-normal-repeat.png"
$debug = Join-Path $packet "same-state-debug.png"
$early = Join-Path $packet "motion-tick-1.png"
$middle = Join-Path $packet "motion-tick-61.png"
$late = Join-Path $packet "motion-tick-121.png"
$contact = Join-Path $packet "motion-contact-sheet.png"
$state1 = Join-Path $packet "state-tick-1.json"
$state61 = Join-Path $packet "state-tick-61.json"
$state61Repeat = Join-Path $packet "state-tick-61-repeat.json"
$state61Debug = Join-Path $packet "state-tick-61-debug.json"
$state121 = Join-Path $packet "state-tick-121.json"

try {
    $installation = Import-VisualStudioEnvironment
    $cmake = Join-Path $installation "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $ctest = Join-Path $installation "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
    Invoke-Checked "configure-release" $cmake @("--preset", "release") $repoRoot
    Invoke-Checked "build-release" $cmake @("--build", "--preset", "release") $repoRoot
    Invoke-Checked "ctest-release" $ctest @("--preset", "release") $repoRoot -RecordState
    $executable = Join-Path $repoRoot "build\Windows\release\wide_eye.exe"
    Invoke-Checked "allocation-oracle-fixed-update" `
        (Join-Path $repoRoot "build\Windows\release\wide_eye_gameplay_simulation_tests.exe") @() -RecordState
    Invoke-Checked "allocation-oracle-presentation" `
        (Join-Path $repoRoot "build\Windows\release\wide_eye_sheep_proxy_tests.exe") @() -RecordState

    function Capture-Motion([string]$Stage, [int]$Tick, [string]$View,
                            [string]$CapturePath, [string]$StatePath) {
        Invoke-Checked $Stage $executable @("--sheep-motion-render-smoke", "--tick", "$Tick",
            "--view", $View, "--capture", $CapturePath, "--state-dump", $StatePath) -RecordState
    }
    Capture-Motion "motion-tick-1" 1 "normal" $early $state1
    Capture-Motion "same-state-normal" 61 "normal" $normal $state61
    Capture-Motion "same-state-normal-repeat" 61 "normal" $repeat $state61Repeat
    Capture-Motion "same-state-debug" 61 "debug" $debug $state61Debug
    Capture-Motion "motion-tick-61" 61 "normal" $middle $state61
    Capture-Motion "motion-tick-121" 121 "normal" $late $state121
    Invoke-Checked "five-proxy-performance" $executable @("--sheep-motion-performance-smoke") -RecordState
    if ($script:Observed["within_provisional_low_budget"] -ne "yes") {
        throw "The five-proxy scene exceeded a provisional Low budget."
    }
    if ($script:Observed["performance_budget_id"] -ne "tracer2-low-profile-v1") {
        throw "The five-proxy scene did not use the Tracer 2 performance budget."
    }

    if ((Get-FileHash $normal -Algorithm SHA256).Hash -ne
        (Get-FileHash $repeat -Algorithm SHA256).Hash) {
        throw "Repeated same-state normal captures differ."
    }
    if ((Get-FileHash $state61 -Algorithm SHA256).Hash -ne
        (Get-FileHash $state61Repeat -Algorithm SHA256).Hash -or
        (Get-FileHash $state61 -Algorithm SHA256).Hash -ne
        (Get-FileHash $state61Debug -Algorithm SHA256).Hash) {
        throw "Same-state canonical state dumps differ."
    }
    New-ContactSheet @($early, $middle, $late) $contact
    $script:Observed["same_state_capture_repeat"] = "yes"
    $script:Observed["same_state_dump_repeat"] = "yes"
    $result = "pass"
}
catch {
    $failure = [ordered]@{ message = $_.Exception.Message }
    Write-Logged "failure_stage=tracer2_presentation_runner"
    Write-Logged ("failure_message={0}" -f $_.Exception.Message)
}

$git = Get-Command git.exe -ErrorAction SilentlyContinue
$commit = $null
$status = @()
if ($null -ne $git) {
    $safeRoot = $repoRoot.Replace("\", "/")
    $commit = (& $git.Source -c "safe.directory=$safeRoot" -C $repoRoot rev-parse HEAD).Trim()
    $status = @(& $git.Source -c "safe.directory=$safeRoot" -C $repoRoot status --short)
}
$configuration = [ordered]@{
    schema_version = 1; build_preset = "release"
    scenario = "presentation-motion"; scenario_version = 1; seed = 0
    state_dump_version = 2; interpolation_alpha = 0.5
    capture_viewport = [ordered]@{ width = 1920; height = 1080 }
    capture_ticks = @(1, 61, 121); same_state_debug = "face_normals"
    performance = [ordered]@{ viewport = "1920x1080"; warmup_frames = 120; sample_frames = 600
        timing_mode = "serialized_gpu_query_and_swap"; budget_id = "tracer2-low-profile-v1" }
}
$configurationPath = Join-Path $packet "configuration.json"
$statePath = Join-Path $packet "measurements.json"
$sourceHashesPath = Join-Path $packet "source-hashes.json"
Write-JsonFile $configurationPath $configuration
Write-JsonFile $statePath ([pscustomobject]$script:Observed)
$hashInputs = @(
    (Join-Path $repoRoot "CMakeLists.txt"),
    (Join-Path $repoRoot "CMakePresets.json"),
    $PSCommandPath
)
foreach ($directory in "cmake", "src", "tests", "third_party") {
    $hashInputs += Get-ChildItem -LiteralPath (Join-Path $repoRoot $directory) -File -Recurse |
        Select-Object -ExpandProperty FullName
}
$sourceHashes = @($hashInputs | Sort-Object -Unique | ForEach-Object {
    [pscustomobject][ordered]@{
        path = $_.Substring($repoRoot.Length).TrimStart("\", "/").Replace("\", "/")
        sha256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})
Write-JsonFile $sourceHashesPath ([ordered]@{ schema_version = 1; files = $sourceHashes })

$records = [System.Collections.Generic.List[object]]::new()
Add-Artifact $records "log" $script:LogPath "text/plain" $packet
Add-Artifact $records "configuration" $configurationPath "application/json" $packet
Add-Artifact $records "measurements" $statePath "application/json" $packet
Add-Artifact $records "source_hashes" $sourceHashesPath "application/json" $packet
foreach ($entry in @(
    @("same_state_normal", $normal, "image/png"), @("same_state_normal_repeat", $repeat, "image/png"),
    @("same_state_debug", $debug, "image/png"), @("motion_tick_1", $early, "image/png"),
    @("motion_tick_61", $middle, "image/png"), @("motion_tick_121", $late, "image/png"),
    @("motion_contact_sheet", $contact, "image/png"), @("state_tick_1", $state1, "application/json"),
    @("state_tick_61", $state61, "application/json"), @("state_tick_121", $state121, "application/json"))) {
    Add-Artifact $records $entry[0] $entry[1] $entry[2] $packet
}
$manifest = [ordered]@{
    schema = "wide-eye.artifact-manifest"; schema_version = 1
    packet_version = "tracer2-presentation-review-v1"; result = $result
    started_at_utc = $started.ToString("o"); finished_at_utc = (Get-Date).ToUniversalTime().ToString("o")
    source = [ordered]@{ commit = $commit; worktree_state = if ($status.Count) { "dirty" } else { "clean" }; status = $status }
    platform = [ordered]@{
        name = "native-windows"
        os = (Get-CimInstance Win32_OperatingSystem).Caption
        cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1).Name
        gpu_inventory = @(Get-CimInstance Win32_VideoController | ForEach-Object {
            [ordered]@{ name = $_.Name; driver_version = $_.DriverVersion }
        })
    }
    configuration = "configuration.json"; measurements = "measurements.json"
    source_hashes = "source-hashes.json"; review_document = "review.md"
    reproduction_command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    workspace_local = $true; commands = @($script:Commands); failure = $failure; artifacts = @($records)
}
$manifestPath = Join-Path $packet "manifest.json"
Write-JsonFile $manifestPath $manifest

$review = @"
# Tracer 2 five-proxy presentation candidate review

This is a candidate evidence packet. It does not accept final sheep art or flock behavior, and its owner verdict is intentionally blank.

## Review question

Are five recognizable procedural sheep, their facing and motion, the same-state debug view, and the measured whole-scene envelope representative enough to begin flock-behavior iteration?

## Automated evidence

| Check | Observed result | Evidence |
| --- | --- | --- |
| Release suite | $result | run.log |
| Same-state repeat | capture $($script:Observed["same_state_capture_repeat"]); canonical v2 state $($script:Observed["same_state_dump_repeat"]) | same-state-normal.png, same-state-normal-repeat.png, state-tick-61.json |
| Snapshot/presentation preparation | median $($script:Observed["snapshot_presentation_preparation_median_ns"]) ns; p95 $($script:Observed["snapshot_presentation_preparation_p95_ns"]) ns; p99 $($script:Observed["snapshot_presentation_preparation_p99_ns"]) ns | measurements.json |
| Render submission | median $($script:Observed["cpu_submission_median_ns"]) ns; p95 $($script:Observed["cpu_submission_p95_ns"]) ns; p99 $($script:Observed["cpu_submission_p99_ns"]) ns | measurements.json |
| GPU render | median $($script:Observed["gpu_render_median_ns"]) ns; p95 $($script:Observed["gpu_render_p95_ns"]) ns; p99 $($script:Observed["gpu_render_p99_ns"]) ns | measurements.json |
| Synchronized frame | median $($script:Observed["synchronized_frame_median_ns"]) ns; p95 $($script:Observed["synchronized_frame_p95_ns"]) ns; p99 $($script:Observed["synchronized_frame_p99_ns"]) ns | measurements.json |
| Allocation oracle | fixed update $($script:Observed["steady_state_allocations"]); snapshot/presentation preparation $($script:Observed["snapshot_presentation_preparation_allocations"]) | release CTest output in run.log |
| Process memory | current $($script:Observed["process_rss_bytes"]) bytes; peak $($script:Observed["process_peak_rss_bytes"]) bytes | measurements.json |
| Tracer 2 Low budget | $($script:Observed["performance_budget_id"]): $($script:Observed["within_provisional_low_budget"]) on this proxy host | configuration.json, measurements.json |

Known limits: the scripted motion is presentation evidence, not flock behavior; sheep are deliberate procedural proxies; the debug view diagnoses paddock surface normals while preserving the identical authoritative sheep state; synchronized timings serialize each GPU query and include swap; the available Intel UHD 630 is a proxy, not the named Iris Xe Low device; native Linux graphics remain unverified.

## Owner verdict

- [ ] **Accept** - representative enough for gameplay iteration with the limits above.
- [ ] **Revise**
- [ ] **Reject**

**Owner observation and required follow-up:**

**Owner/date:**
"@
[System.IO.File]::WriteAllText((Join-Path $packet "review.md"), $review + [Environment]::NewLine,
    [System.Text.UTF8Encoding]::new($false))

[Console]::Out.WriteLine("artifact_manifest=$manifestPath")
[Console]::Out.WriteLine("visual_review_packet=$(Join-Path $packet 'review.md')")
[Console]::Out.WriteLine("tracer2_presentation_result=$result")
if ($result -ne "pass") { exit 1 }
