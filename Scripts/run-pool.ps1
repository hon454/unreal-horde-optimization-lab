# 엔티티 풀 긴 실행 러너(v2, 묶음 Pool<n>). 최종 구성(단계 8, 조건 s8, 풀 켬)과 풀을 끈 조건(a8pool, -LabEntityPool=0)을 긴 측정 구간(기본 960초)으로 잰다.
# 시나리오는 총괄 묶음과 같다: 1만, 이동 장면(V0, 오브 궤도 2m/s), 카메라 P1, 워밍업 60초. 바꾸는 것은 측정 시간뿐이다.
# 시간 추이는 프레임 CSV의 Lab/IskmTracks, GPUMem/LocalUsedMB로 본다(Scripts/pool-timeline.py가 1분 구간 값을 Measurements/<묶음>/timeline.csv에 쓴다).
# 조건 인자, 기대 메타데이터, 시나리오 값은 Scripts/full-conditions.json(v2)에서 읽는다. 절차(앞 창 검사, 앞 창 감시, CPU 클럭 기록, 메타데이터 대조, 실패 CSV 이동, progress.log,
#   failures.csv, 실패 재측정, -Resume, 연속 실패 중단, dirty 패키지 거부)는 run-full.ps1에서 옮겼다. run-full.ps1과 조건 표는 고치지 않는다.
# v1 러너(조건 L8-q4final, A8-pool을 읽던 것)는 태그 v1에 있다. Pool1(v1)은 이 러너로 다시 만들 수 없다.
# 라벨: <묶음>-V0-<구성>-N<N>-P1-r<회차>(총괄 묶음과 같은 형식이라 summarize.py가 읽는다). 구성은 s8, a8pool.
# 실행 순서: 회차가 바깥 루프이고 짝수 회차는 구성 순서를 뒤집는다(-Runs 2면 r1 s8, a8pool / r2 a8pool, s8). 기본은 1회씩이다(v1 Pool1과 같다). 묶음 맨 첫 실행 r0은 두지 않는다(긴 실행이라 한 번이 17분이고, v1도 없었다).
# 실패한 실행은 Measurements/<묶음>/failures.csv에 적고, 묶음 끝에서 실패분만 새 회차 번호(회차 + Runs)로 한 번 다시 잰다. 실행이 연달아 -MaxConsecFail회(기본 2) 실패하면 묶음을 멈춘다(종료 코드 2).
# 끝에서 summarize.py(runs.csv, summary.csv)와 pool-timeline.py(timeline.csv)를 부른다. 실행이 한 번씩이라 범위가 없다. 차이 판정에 쓰지 않고 시간 추이만 본다.
# -File로 부르면 배열 인자가 한 문자열로 넘어오므로 -Conds는 쉼표로 이은 문자열이다.
param(
    [Parameter(Mandatory = $true)][string]$Batch,
    [int]$N = 10000,
    # 측정 구간(초). 총괄 묶음은 180이다. v1 Pool1은 960이었다
    [int]$DurationSec = 960,
    # 쉼표로 이은 구성 이름(조건 표). 풀 켬 = s8, 풀 끔 = a8pool
    [string]$Conds = "s8,a8pool",
    # 구성마다 잴 회차 수
    [int]$Runs = 1,
    [switch]$Resume,
    [switch]$DryRun,
    [switch]$NoFgCheck,
    [switch]$NoClock,
    # 예상 시간 계산용: 프로세스 시작부터 워밍업 시작까지(초)와 시간 제한 배수
    [int]$StartupSec = 30,
    [int]$TimeoutFactor = 2,
    # 시험용: 패키지 커밋에 dirty가 있어도 시작한다. 측정 묶음에는 쓰지 않는다
    [switch]$AllowDirty,
    # 시험용: 창이 열린 chrome, msedge, firefox가 있어도 시작한다. 측정 묶음에는 쓰지 않는다(Chrome의 CPU 버스트가 7초 주기 스파이크를 만든다)
    [switch]$AllowBrowsers,
    # 실행이 연달아 이 횟수만큼 실패하면 묶음을 멈춘다(종료 코드 2). 0이면 끈다
    [int]$MaxConsecFail = 2
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"
$PSNativeCommandUseErrorActionPreference = $false

$FlushSec = 2
$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Doc.version -ne 2) { Write-Host "FAIL: full-conditions.json is not version 2."; exit 1 }
$Sc = $Doc.scenario
$Speed = "V0"
$Sp = $Sc.speeds.$Speed
$Warmup = [int]$Sp.warmup_sec
$Mps = [string]$Sp.mps
$Preset = 1
$RunSec = $StartupSec + $Warmup + $DurationSec + $FlushSec
$TimeoutSec = $TimeoutFactor * $RunSec

$CondList = @($Conds -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$Cfgs = @()
foreach ($name in $CondList) {
    $c = $Doc.configs | Where-Object { $_.name -eq $name } | Select-Object -First 1
    if (-not $c) { Write-Host "FAIL: no config '$name' in full-conditions.json."; exit 1 }
    $Cfgs += $c
}
if ($Cfgs.Count -eq 0) { Write-Host "FAIL: no configs selected."; exit 1 }

function New-Job($Cfg, $Run) {
    return @{ Cfg = $Cfg; Run = $Run; Label = "$Batch-$Speed-$($Cfg.name)-N$N-P$Preset-r$Run"; RetryRun = $Run + $Runs }
}
function Retry-Label($j) { return "$Batch-$Speed-$($j.Cfg.name)-N$N-P$Preset-r$($j.RetryRun)" }
$All = New-Object System.Collections.ArrayList
for ($r = 1; $r -le $Runs; $r++) {
    $list = if ($r % 2 -eq 0) { $r2 = @($Cfgs); [array]::Reverse($r2); $r2 } else { $Cfgs }
    foreach ($c in $list) { [void]$All.Add((New-Job $c $r)) }
}

function Get-GameArgs($j, $label) {
    # common_args의 -LabPreset=1은 아래에서 다시 준다(run-full.ps1과 같은 순서). 측정 구간만 -LabDuration으로 늘린다
    $a = @($Doc.common_args | Where-Object { $_ -notlike "-LabPreset=*" }) + @($j.Cfg.args)
    $a += @("-LabPreset=$Preset", "-LabN=$N", "-LabLabel=$label", "-LabOrbOrbitRadius=$($Sc.radius_m)", "-LabOrbOrbitSpeed=$Mps", "-LabOrbHeight=$($Sc.height_m)",
            "-LabWarmup=$Warmup", "-LabDuration=$DurationSec")
    return $a
}

$Exe = "$ProjectDir\$(($Doc.exe) -replace '/', '\')"
$PkgRoot = "$ProjectDir\Packaged\Test\Windows\HordeOptLab"
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"
$ClockCsv = "$BatchDir\clock.csv"
$BatchJson = "$BatchDir\batch.json"
$CommitFile = "$ProjectDir\Packaged\Test\commit.txt"
$Commit = if (Test-Path $CommitFile) { ((Get-Content $CommitFile -Encoding UTF8 -TotalCount 1)).TrimStart([char]0xFEFF).Trim() } else { "unknown" }
$Build12 = if ($Commit -match "^[0-9a-f]{12}") { $Commit.Substring(0, 12) } else { "" }

# ---- 예행 출력 ----
if ($DryRun) {
    $estMin = [math]::Round($All.Count * $RunSec / 60.0, 1)
    Write-Host "DRYRUN batch=$Batch n=$N speed=$Speed warmup=${Warmup}s measure=${DurationSec}s runs=$($All.Count) est=$estMin min (startup $StartupSec + warmup + measure + flush $FlushSec s per run, retries not included) timeout=${TimeoutSec}s build=$Build12 ($Commit)"
    Write-Host "exe: $Exe (exists=$(Test-Path $Exe))"
    if ($Commit -match "dirty") { Write-Host "NOTE: the package commit is dirty; the real run refuses to start unless -AllowDirty (tests only)." }
    foreach ($j in $All) { Write-Host "  $($j.Label)"; Write-Host "      `"$Exe`" $((Get-GameArgs $j $j.Label) -join ' ')" }
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) "lab-pool-dryrun-labels.txt"
    Set-Content -Path $tmp -Value @($All | ForEach-Object { $_.Label }) -Encoding UTF8
    $chk = (& py -3 -I "$PSScriptRoot\ablation-check.py" --labels $tmp 2>&1 | Out-String)
    $labelsExit = $LASTEXITCODE
    Write-Host $chk.TrimEnd()
    Remove-Item $tmp -ErrorAction SilentlyContinue
    Write-Host "  [check] py -3 -I Scripts/ablation-check.py <csv> --build $Build12 --measure-sec $DurationSec"
    if ($labelsExit -ne 0) { Write-Host "DRYRUN FAIL"; exit 1 }
    exit 0
}

# ---- 시작 검사 ----
if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe. Run Scripts/package.ps1 first."; exit 1 }
$Hard = @("UnrealEditor", "UnrealEditor-Cmd", "HordeOptLab-Win64-Test", "ShaderCompileWorker", "UnrealBuildTool", "ffmpeg")
$Soft = @("chrome", "msedge", "firefox")
$Running = @(Get-Process -Name ($Hard + $(if ($AllowBrowsers) { @() } else { $Soft })) -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) { Write-Host "FAIL: other programs are up ($(($Running | ForEach-Object { "$($_.ProcessName) pid $($_.Id)" }) -join ', ')). Close them first (chrome causes 7s periodic spikes)."; exit 1 }
if ($AllowBrowsers) {
    $Br = @(Get-Process -Name $Soft -ErrorAction SilentlyContinue)
    if ($Br.Count -gt 0) { Write-Host "NOTE: -AllowBrowsers; browsers are up ($(($Br | Group-Object ProcessName | ForEach-Object { "$($_.Name) x$($_.Count)" }) -join ', ')). Do not use it for a measurement batch." }
}
if ($Commit -match "dirty") {
    if ($AllowDirty) { Write-Host "NOTE: the package is dirty ($Commit); -AllowDirty lets the batch start. Do not use it for a measurement batch." }
    else { Write-Host "FAIL: the package commit is dirty ($Commit). The Test package must be clean (tag v2-full4). -AllowDirty is for tests only."; exit 1 }
}
if ($Build12 -eq "") { Write-Host "NOTE: no commit hash in $CommitFile; LabBuild is not checked." }

if (-not ('FgWin' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class FgWin {
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
"@
}
function Get-FgPid {
    $p = [uint32]0
    [void][FgWin]::GetWindowThreadProcessId([FgWin]::GetForegroundWindow(), [ref]$p)
    return [int]$p
}
# 게임 창을 앞에 둔다(run-full.ps1과 같다). 1) MainWindowHandle을 최대 30초 기다린다 2) SetForegroundWindow 3) AppActivate(PID) 4) Alt 키 눌림 뒤 SetForegroundWindow
function Set-GameForeground($Proc) {
    $Deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $Deadline) {
        $Proc.Refresh()
        if ($Proc.HasExited) { return "focus=exited" }
        if ($Proc.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }
    if ($Proc.MainWindowHandle -eq [IntPtr]::Zero) { return "focus=NOWINDOW" }
    $H = $Proc.MainWindowHandle
    [void][FgWin]::SetForegroundWindow($H); Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=ok" }
    try { [void](New-Object -ComObject WScript.Shell).AppActivate($Proc.Id) } catch { }
    Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=appactivate" }
    [FgWin]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); [FgWin]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
    [void][FgWin]::SetForegroundWindow($H); Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=altkey" }
    return "focus=FAILED"
}

# CPU 카운터. 못 만들면 클럭 기록만 건너뛴다(측정은 계속). % Processor Performance는 기본 클럭 대비 %다
$ClkPerf = $null; $ClkFreq = $null; $ClkBusy = $null; $BaseMhz = 0.0
if (-not $NoClock) {
    try {
        $ClkPerf = New-Object System.Diagnostics.PerformanceCounter("Processor Information", "% Processor Performance", "_Total")
        $ClkFreq = New-Object System.Diagnostics.PerformanceCounter("Processor Information", "Processor Frequency", "_Total")
        $ClkBusy = New-Object System.Diagnostics.PerformanceCounter("Processor Information", "% Processor Time", "_Total")
        [void]$ClkPerf.NextValue(); [void]$ClkBusy.NextValue(); $BaseMhz = [double]$ClkFreq.NextValue()
    } catch { Write-Host "NOTE: CPU counters unavailable ($($_.Exception.Message)); clock.csv is not written."; $ClkPerf = $null; $ClkBusy = $null }
}
function Get-CpuBusy {
    if (-not $ClkBusy) { return "" }
    [void]$ClkBusy.NextValue(); Start-Sleep -Seconds 3
    return [math]::Round([double]$ClkBusy.NextValue(), 1)
}

New-Item -ItemType Directory -Force -Path $OutCsv, $BatchDir | Out-Null

# ---- 진행 기록 ----
$Status = @{}   # 라벨 -> OK|FAIL
if (Test-Path $ProgressLog) {
    if (-not $Resume) { Write-Host "FAIL: $ProgressLog exists. Use -Resume to continue, or a new -Batch name (labels are never reused)."; exit 1 }
    foreach ($line in (Get-Content $ProgressLog -Encoding UTF8)) {
        $f = $line.Split("`t")
        if ($f.Count -ge 3) { $Status[$f[1]] = $f[2] }
    }
} elseif ($Resume) { Write-Host "NOTE: -Resume but no progress.log; starting fresh." }
if (-not $Resume) {
    # 같은 라벨의 CSV가 이미 있으면 거부(AGENTS.md "같은 라벨은 다시 쓰지 않는다")
    $dups = @($All | Where-Object { Test-Path "$OutCsv\$($_.Label).csv" } | ForEach-Object { $_.Label })
    if ($dups.Count -gt 0) { Write-Host "FAIL: CSV already exists for $($dups.Count) labels, e.g. $($dups[0]). Use -Resume or a new -Batch."; exit 1 }
}
if (-not (Test-Path $FailuresCsv)) { Set-Content -Path $FailuresCsv -Value "time,label,cond,n,preset,kind,reason" -Encoding UTF8 }
if (-not (Test-Path $ClockCsv)) { Set-Content -Path $ClockCsv -Value "label,samples,perf_mean_pct,perf_min_pct,perf_max_pct,mhz_mean,cpu_busy_before_pct" -Encoding UTF8 }

function Now { return (Get-Date -Format "yyyy-MM-dd HH:mm:ss") }
function Clean($s) { return (($s -replace "[\r\n]+", " ") -replace ",", ";").Trim() }
function Write-Progress-Line($Label, $St, $P50, $P95, $Reason) {
    $Status[$Label] = $St
    Add-Content -Path $ProgressLog -Encoding UTF8 -Value ("{0}`t{1}`t{2}`t{3}`t{4}`t{5}" -f (Now), $Label, $St, $P50, $P95, (Clean $Reason))
}
function Write-Failure($j, $Label, $Reason) {
    Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},{2},{3},P{4},pool,{5}" -f (Now), $Label, "$Speed-$($j.Cfg.name)", $N, $Preset, (Clean $Reason))
}

$Started = Now
$Resumes = 0
if ($Resume -and (Test-Path $BatchJson)) {
    try { $old = Get-Content $BatchJson -Raw -Encoding UTF8 | ConvertFrom-Json; $Started = $old.started; $Resumes = [int]$old.resumes + 1 } catch { }
}
function Save-BatchJson($Ended, $State) {
    $ok = @($All | Where-Object { $Status[$_.Label] -eq "OK" -or $Status[(Retry-Label $_)] -eq "OK" }).Count
    $obj = [ordered]@{
        batch = $Batch; state = $State; kind = "entity-pool-long-run"; commit = $Commit; started = $Started; ended = $Ended; resumes = $Resumes
        n = $N; speed = $Speed; warmup_sec = $Warmup; measure_sec = $DurationSec; configs = ($CondList -join ","); runs_per_config = $Runs
        orbit = "r$($Sc.radius_m)/h$($Sc.height_m)/v$Mps"; base_mhz = $BaseMhz
        planned_runs = $All.Count; planned_runs_ok = $ok; planned_runs_missing = ($All.Count - $ok)
        giveups = @($All | Where-Object { $Status[$_.Label] -eq "FAIL" -and $Status[(Retry-Label $_)] -eq "FAIL" } | ForEach-Object { $_.Label })
    }
    $obj | ConvertTo-Json -Depth 4 | Set-Content -Path $BatchJson -Encoding UTF8
}

# ---- 실행 하나 ----
function Invoke-Run($j, $Label) {
    $res = @{ Ok = $false; Reason = ""; P50 = ""; P95 = "" }
    $pkgCsvFile = "$PkgCsv\$Label.csv"
    $summary = "$PkgRoot\Saved\Lab\$Label.summary.txt"
    try {
        Remove-Item $pkgCsvFile, $summary -ErrorAction SilentlyContinue
        # 시작 전 CPU가 한가한지 본다(20% 넘으면 10초씩 최대 5번 기다린다). 값은 clock.csv에 남긴다
        $busy = Get-CpuBusy
        for ($i = 0; $busy -ne "" -and [double]$busy -gt 20 -and $i -lt 5; $i++) { Write-Host "WAIT cpu busy $busy% before $Label"; Start-Sleep -Seconds 10; $busy = Get-CpuBusy }
        Write-Host "RUN $Label $(Get-Date -Format 'HH:mm:ss') cpu_busy_before=$busy"
        $Proc = Start-Process -FilePath $Exe -ArgumentList (Get-GameArgs $j $Label) -PassThru
        $Focus = "focus=skipped"
        $Samples = New-Object System.Collections.ArrayList
        if (-not $NoFgCheck) { $Focus = Set-GameForeground $Proc; Write-Host $Focus }
        $Deadline = (Get-Date).AddSeconds($TimeoutSec)
        $TimedOut = $false
        if ($ClkPerf) { [void]$ClkPerf.NextValue() }
        while (-not $Proc.HasExited) {
            if ((Get-Date) -gt $Deadline) { $TimedOut = $true; Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue; break }
            if (-not $NoFgCheck) {
                $perf = if ($ClkPerf) { [double]$ClkPerf.NextValue() } else { -1.0 }
                [void]$Samples.Add(@{ T = (Get-Date); Pid = (Get-FgPid); Perf = $perf })
            }
            [void]$Proc.WaitForExit(2000)
        }
        $Proc.WaitForExit()
        $Exit = $Proc.ExitCode
        $failDir = "$OutCsv\failed"
        $moveFail = {
            param($dir)
            if (Test-Path $pkgCsvFile) { New-Item -ItemType Directory -Force -Path $dir | Out-Null; Move-Item $pkgCsvFile "$dir\$Label-$(Get-Date -Format 'yyyyMMddHHmmss').csv" -Force }
        }
        if ($TimedOut) { & $moveFail $failDir; $res.Reason = "timeout ${TimeoutSec}s, process killed"; return $res }
        # 측정 구간 = summary.txt 마지막 기록 시각 - measure_sec. 그 안의 표본 중 게임 PID가 아닌 것을 센다
        $FgFail = $false; $FgNote = ""
        if (-not $NoFgCheck -and (Test-Path $summary)) {
            $Text = Get-Content $summary -Raw
            $Sec = [double]([regex]::Match($Text, "measure_sec=([0-9.]+)").Groups[1].Value)
            $End = (Get-Item $summary).LastWriteTime
            $In = @($Samples | Where-Object { $_.T -ge $End.AddSeconds(-$Sec) -and $_.T -le $End })
            $Non = @($In | Where-Object { $_.Pid -ne $Proc.Id }).Count
            $FgNote = "fg=$($In.Count - $Non)/$($In.Count) $Focus"
            Add-Content -Path $summary -Value ("`n" + $FgNote)
            if ($In.Count -eq 0 -or $Non -gt 0) { $FgFail = $true }
            # 카운터가 가끔 0을 돌려준다. 0은 뺀다
            $pv = @($In | Where-Object { $_.Perf -gt 0 } | ForEach-Object { $_.Perf })
            if ($pv.Count -gt 0) {
                $m = ($pv | Measure-Object -Average -Minimum -Maximum)
                Add-Content -Path $ClockCsv -Encoding UTF8 -Value ("{0},{1},{2:F1},{3:F1},{4:F1},{5:F0},{6}" -f $Label, $pv.Count, $m.Average, $m.Minimum, $m.Maximum, ($m.Average * $BaseMhz / 100.0), $busy)
            }
        }
        if (Test-Path $summary) { Get-Content $summary | Select-Object -Last 4 | Out-Host }
        if ($FgFail) { & $moveFail "$OutCsv\fg-failed"; $res.Reason = "foreground not on game during measurement ($FgNote)"; return $res }
        if ($Exit -ne 0) { & $moveFail $failDir; $res.Reason = "exit code $Exit"; return $res }
        if (-not (Test-Path $pkgCsvFile)) { $res.Reason = "no csv in package Saved/Profiling/CSV"; return $res }
        # 메타데이터 대조(패키지 CSV에서). 측정 구간이 늘어 LabWindow가 60+<초>다(--measure-sec). 틀리면 실행 실패
        $ck = @("-3", "-I", "$ProjectDir\Scripts\ablation-check.py", $pkgCsvFile, "--measure-sec", "$DurationSec")
        if ($Build12) { $ck += @("--build", $Build12) }
        $out = (& py @ck 2>&1 | Out-String)
        $ckExit = $LASTEXITCODE
        Write-Host $out.TrimEnd()
        $mm = [regex]::Match($out, "frames=(\d+) frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
        if ($mm.Success) { $res.P50 = $mm.Groups[2].Value; $res.P95 = $mm.Groups[3].Value }
        if ($ckExit -ne 0) {
            & $moveFail $failDir
            $res.Reason = "metadata check failed: " + (([regex]::Match($out, "CHECK [^\r\n]*FAIL[^\r\n]*")).Value)
            return $res
        }
        if (-not $mm.Success -or [int]$mm.Groups[1].Value -lt 100) { & $moveFail $failDir; $res.Reason = "too few frames or unparsable check output"; return $res }
        Move-Item $pkgCsvFile "$OutCsv\$Label.csv"
        $res.Ok = $true
        return $res
    } catch {
        $res.Reason = "exception: $($_.Exception.Message)"
        try { if ($Proc -and -not $Proc.HasExited) { Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue } } catch { }
        try { if (Test-Path $pkgCsvFile) { New-Item -ItemType Directory -Force -Path "$OutCsv\failed" | Out-Null; Move-Item $pkgCsvFile "$OutCsv\failed\$Label-$(Get-Date -Format 'yyyyMMddHHmmss').csv" -Force } } catch { }
        return $res
    }
}

function Run-Job($j, $Label) {
    $r = Invoke-Run $j $Label
    if ($r.Ok) {
        Write-Progress-Line $Label "OK" $r.P50 $r.P95 ""
        Write-Host "OK $Label p50=$($r.P50) p95=$($r.P95)"
    } else {
        Write-Progress-Line $Label "FAIL" $r.P50 $r.P95 $r.Reason
        Write-Failure $j $Label $r.Reason
        Write-Host "FAIL $Label : $($r.Reason)"
    }
    return $r.Ok
}

# 연속 실패 수. 패키지·빌드 불일치, 메타데이터 키 불일치, 앞 창 빼앗김 같은 계통 오류로 시간을 낭비하지 않게 멈춘다
$ConsecFail = 0
function Note-Result($Ok, $Label) {
    if ($Ok) { $script:ConsecFail = 0; return }
    $script:ConsecFail++
    if ($MaxConsecFail -le 0 -or $script:ConsecFail -lt $MaxConsecFail) { return }
    $why = "$($script:ConsecFail) consecutive failed runs (last $Label)"
    Add-Content -Path $ProgressLog -Encoding UTF8 -Value ("{0}`t-`tABORT`t`t`t{1}" -f (Now), (Clean $why))
    Save-BatchJson (Now) "aborted"
    Write-Host "ABORT $Batch : $why. See failures.csv, fix the cause, then continue with -Resume (failed runs are measured again in the retry pass)."
    exit 2
}

Save-BatchJson "" "running"
Write-Host "BATCH $Batch commit=$Commit runs=$($All.Count) configs=$($CondList -join '/') measure=${DurationSec}s est=$([math]::Round($All.Count * $RunSec / 60.0, 1)) min start=$(Now)"

foreach ($j in $All) {
    if ($Status.ContainsKey($j.Label)) { Write-Host "SKIP $($j.Label) ($($Status[$j.Label]))"; continue }
    if (Test-Path "$OutCsv\$($j.Label).csv") {
        # -Resume 중 CSV는 옮겨졌는데 progress.log에 못 적은 경우. 대조만 다시 한다
        $ck = @("-3", "-I", "$ProjectDir\Scripts\ablation-check.py", "$OutCsv\$($j.Label).csv", "--measure-sec", "$DurationSec"); if ($Build12) { $ck += @("--build", $Build12) }
        $out = (& py @ck 2>&1 | Out-String)
        $ckExit = $LASTEXITCODE
        $m = [regex]::Match($out, "frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
        if ($ckExit -eq 0) { Write-Progress-Line $j.Label "OK" $m.Groups[1].Value $m.Groups[2].Value "recovered csv"; Write-Host "RECOVERED $($j.Label)"; continue }
        New-Item -ItemType Directory -Force -Path "$OutCsv\failed" | Out-Null
        Move-Item "$OutCsv\$($j.Label).csv" "$OutCsv\failed\$($j.Label)-$(Get-Date -Format 'yyyyMMddHHmmss').csv" -Force
        Write-Progress-Line $j.Label "FAIL" "" "" "orphan csv failed check"; Write-Failure $j $j.Label "orphan csv failed check"; Note-Result $false $j.Label; continue
    }
    Note-Result (Run-Job $j $j.Label) $j.Label
}

# 실패분만 새 회차 번호(회차 + Runs)로 한 번 다시 잰다. 다시 실패하면 포기
$Retry = @($All | Where-Object { $Status[$_.Label] -eq "FAIL" })
if ($Retry.Count -gt 0) { Write-Host "RETRY PASS: $($Retry.Count) failed runs" }
foreach ($j in $Retry) {
    $rl = Retry-Label $j
    if ($Status.ContainsKey($rl)) { continue }
    $jr = New-Job $j.Cfg $j.RetryRun
    $ok = Run-Job $jr $jr.Label
    if (-not $ok) { Write-Host "GIVEUP $($jr.Label) (failed twice: $($j.Label))" }
    Note-Result $ok $jr.Label
}

$Ended = Now
Save-BatchJson $Ended "done"
$okCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "OK" }).Count
$failCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "FAIL" }).Count
Write-Host "DONE $Batch ok=$okCount fail=$failCount ended=$Ended (details: $BatchJson)"

# 요약(runs.csv, summary.csv)과 1분 구간 값(timeline.csv). 실행이 한 번씩이라 범위가 없다
& py -3 -I "$PSScriptRoot\summarize.py" --batch $Batch 2>&1 | Out-Host
& py -3 -I "$PSScriptRoot\pool-timeline.py" --batch $Batch 2>&1 | Out-Host
$planned = @($All | Where-Object { $Status[$_.Label] -eq "OK" -or $Status[(Retry-Label $_)] -eq "OK" }).Count
if ($planned -lt $All.Count) { Write-Host "BATCH INCOMPLETE: $planned of $($All.Count) planned runs are OK (details: $FailuresCsv)."; exit 1 }
exit 0
