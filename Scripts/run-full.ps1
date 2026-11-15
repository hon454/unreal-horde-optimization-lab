# 총괄 묶음 러너(v2 W12). Test 패키지 exe를 직접 부른다. v1의 run-full.ps1(태그 v1)을 v2의 사다리 단계(-LabStage=1~8)와 빼 보기 스위치에 맞게 다시 썼다.
# 조건은 Scripts/full-conditions.json(v2)이 정한다: 구성(단계 s1~s8, 빼 보기 a8*), 묶음 종류(group)별 구성·속도·N·회차, 시나리오 값(궤도, 워밍업, 측정 구간).
# 절차(앞 창 검사, 실패 CSV 이동, progress.log, failures.csv, 메타데이터 대조)는 run-spike.ps1과 v1 run-full.ps1에서 옮겼다.
# 묶음 종류(group): ladder(8단계 x 1만 x 이동), ablation(빼 보기 x 이동), still(단계 8과 빼 보기 x 정지), sub(하위 실험), maxn(Mass 단계 x {5천, 2만}),
#   actor(단계 1 x {500, 1천, 2천}), aa(단계 8 반복, 회차 11~13), trace(단계 1~8 트레이스), traceser(단계 2~8 직렬 트레이스), extra(기본 묶음에 없는 후보, -Only extra로만).
# 라벨: <묶음>-<V0|V1>-<구성>-N<N>-P1-r<회차>. V0는 이동(궤도 속도 2m/s), V1은 정지(0). 트레이스는 <묶음>trace-..., <묶음>traceser-...
# 실행 순서: 회차가 바깥 루프(r1 전 조건 -> r2 -> r3), 짝수 회차는 슬롯 순서를 뒤집는다(표류가 조건에 고르게 퍼지게). -Order cond면 조건 단위. 묶음 맨 첫 실행 r0 하나는 버린다.
# 실행마다 끝난 직후 ablation-check.py로 메타데이터를 대조한다. 실패(종료 코드, CSV 없음, 앞 창이 게임이 아님, 대조 틀림, 시간 초과)는 Measurements/<묶음>/failures.csv에 적고 넘어간다.
# 묶음 끝에서 실패분만 새 회차 번호(회차 + 그 묶음 종류의 회차 수)로 한 번 다시 잰다. 실패 CSV는 Saved/Profiling/CSV/failed/ 또는 fg-failed/로 옮겨 요약에서 빠진다.
# 진행은 Measurements/<묶음>/progress.log(탭 구분: 시각, 라벨, OK|FAIL|DISCARD, 프레임 p50, p95, 이유). -Resume이면 거기 있는 라벨은 건너뛴다(중단 뒤 이어 하기).
# v2 W12: 실행마다 CPU 클럭(% Processor Performance, 측정 구간 평균·최소·최대)과 시작 전 CPU 사용률을 Measurements/<묶음>/clock.csv에 남긴다(summarize.py가 runs.csv에 붙인다).
#   시작 전 CPU가 한가하지 않으면(20% 넘음) 최대 50초 기다렸다 시작한다. 시작 검사는 편집기·게임·브라우저·셰이더 컴파일러 같은 방해 프로세스가 떠 있으면 멈춘다.
# -File로 부르면 배열 인자가 한 문자열로 넘어오므로 -Only, -Pick은 쉼표로 이은 문자열이다.
param(
    [Parameter(Mandatory = $true)][string]$Batch,
    # 쉼표로 이은 묶음 종류. 비우면 기본(ladder, ablation, still, sub, maxn, actor, aa, trace, traceser). extra는 적어야 돈다
    [string]$Only = "",
    # 쉼표로 이은 group:구성(별표 가능). 예: "ladder:s1,still:s8,trace:*". 맞는 슬롯만 남긴다(예행용)
    [string]$Pick = "",
    # 쉼표로 이은 개체 수. 주면 그 N인 슬롯만 남긴다(예행용)
    [string]$NFilter = "",
    # 주면(0 이상) 반복 묶음의 회차 수를 덮어쓴다. 트레이스와 A/A는 JSON 값이다. 기본은 JSON 값(3)
    [int]$Runs = -1,
    [switch]$Resume,
    [switch]$DryRun,
    [switch]$SkipR0,
    [switch]$NoFgCheck,
    [ValidateSet("round", "cond")][string]$Order = "round",
    # 예상 시간 계산용: 프로세스 시작부터 워밍업 시작까지(초)와 시간 제한 배수
    [int]$StartupSec = 30,
    [int]$TimeoutFactor = 3,
    # CPU 클럭 기록을 끈다
    [switch]$NoClock,
    # 러너 시험용(게임 없이 파싱·재시도·이어 하기 점검): 다른 실행 파일과 앞 인자, 패키지 폴더. 측정에는 쓰지 않는다
    [string]$ExePath = "",
    [string]$ExeArgsPrefix = "",
    [string]$PkgRootOverride = "",
    [switch]$NoProcCheck
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"
$PSNativeCommandUseErrorActionPreference = $false

$FlushSec = 2
$ObservedSavingSec = 26   # 묶음 V2W11Sweep의 이동 실행은 진행 로그의 시각 차가 246초로, 위 상한(30 + 60 + 180 + 2 = 272초)보다 26초 짧다. 예상 시간의 두 번째 값

# ---- 조건 JSON에서 슬롯과 작업 목록을 만든다 ----
$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Doc.version -ne 2) { Write-Host "FAIL: full-conditions.json is not version 2."; exit 1 }
$Sc = $Doc.scenario
$Cfg = @{}
foreach ($c in $Doc.configs) { $Cfg[$c.name] = $c }
$DefaultGroups = @($Doc.main_groups) + @("aa", "trace", "traceser")
$OnlyList = @($Only -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$GroupNames = @(if ($OnlyList.Count -gt 0) { $OnlyList } else { $DefaultGroups })
foreach ($g in $GroupNames) { if (-not $Doc.groups.PSObject.Properties[$g]) { Write-Host "FAIL: unknown group '$g' in -Only ($(($Doc.groups.PSObject.Properties.Name) -join ', '))."; exit 1 } }
$PickList = @($Pick -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" } | ForEach-Object { $p = $_ -split ":"; @{ Group = $p[0]; Cfg = $(if ($p.Count -gt 1) { $p[1] } else { "*" }) } })
$NList = @($NFilter -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" } | ForEach-Object { [int]$_ })

# 슬롯 = 묶음 종류 x 구성 x 속도 x N. 같은 슬롯의 회차들이 한 조건의 반복이다
$Slots = New-Object System.Collections.ArrayList
foreach ($gn in $GroupNames) {
    $g = $Doc.groups.$gn
    $isAux = ($gn -in @("aa", "trace", "traceser"))
    $runCount = if (-not $isAux -and $Runs -ge 0) { $Runs } else { [int]$g.runs }
    foreach ($cn in $g.configs) {
        if (-not $Cfg.ContainsKey($cn)) { Write-Host "FAIL: group $gn lists unknown config '$cn'."; exit 1 }
        foreach ($n in $g.n) {
            if ($PickList.Count -gt 0 -and -not @($PickList | Where-Object { ($_.Group -eq "*" -or $_.Group -eq $gn) -and ($_.Cfg -eq "*" -or $_.Cfg -eq $cn) }).Count) { continue }
            if ($NList.Count -gt 0 -and $NList -notcontains [int]$n) { continue }
            $sp = $Sc.speeds.($g.speed)
            $measure = if ($g.measure_sec) { [int]$g.measure_sec } else { [int]$Sc.measure_sec }
            $prefix = $Batch + $(if ($g.prefix) { [string]$g.prefix } else { "" })
            [void]$Slots.Add(@{ Group = $gn; Cfg = $Cfg[$cn]; Speed = [string]$g.speed; Mps = [string]$sp.mps; Warmup = [int]$sp.warmup_sec; Measure = $measure; N = [int]$n
                                Prefix = $prefix; RunBase = $(if ($g.run_base) { [int]$g.run_base } else { 0 }); RunCount = $runCount
                                Trace = [bool]$g.trace; Serial = [bool]$g.serial; Aux = $isAux })
        }
    }
}
function Slot-Key($j) { return "$($j.Slot.Group)|$($j.Slot.Prefix)|$($j.Slot.Speed)|$($j.Slot.Cfg.name)|$($j.Slot.N)" }
function Run-Sec($s) { return $StartupSec + $s.Warmup + $s.Measure + $FlushSec }
function New-Job($Slot, $Run) {
    $label = "$($Slot.Prefix)-$($Slot.Speed)-$($Slot.Cfg.name)-N$($Slot.N)-P1-r$Run"
    return @{ Slot = $Slot; Run = $Run; Label = $label; RetryRun = $Run + $Slot.RunCount; Group = $Slot.Group }
}
$MainSlots = @($Slots | Where-Object { -not $_.Aux })
$AuxSlots = @($Slots | Where-Object { $_.Aux })
$Main = New-Object System.Collections.ArrayList
if ($Order -eq "round") {
    $maxRuns = @($MainSlots | ForEach-Object { $_.RunCount } | Measure-Object -Maximum).Maximum
    for ($r = 1; $r -le $maxRuns; $r++) {
        $list = if ($r % 2 -eq 0) { $r2 = @($MainSlots); [array]::Reverse($r2); $r2 } else { $MainSlots }
        foreach ($s in $list) { if ($r -le $s.RunCount) { [void]$Main.Add((New-Job $s $r)) } }
    }
} else {
    foreach ($s in $MainSlots) { for ($r = 1; $r -le $s.RunCount; $r++) { [void]$Main.Add((New-Job $s $r)) } }
}
$Aux = New-Object System.Collections.ArrayList
foreach ($s in $AuxSlots) { for ($r = 1; $r -le $s.RunCount; $r++) { [void]$Aux.Add((New-Job $s ($s.RunBase + $r))) } }
$All = @($Main) + @($Aux)
$SlotCount = @($All | ForEach-Object { Slot-Key $_ } | Sort-Object -Unique).Count

# ---- 게임 인자 ----
function Get-GameArgs($j, $label) {
    $s = $j.Slot
    $a = @($Doc.common_args) + @($s.Cfg.args)
    $a += @("-LabN=$($s.N)", "-LabLabel=$label", "-LabOrbOrbitRadius=$($Sc.radius_m)", "-LabOrbOrbitSpeed=$($s.Mps)", "-LabOrbHeight=$($Sc.height_m)",
            "-LabWarmup=$($s.Warmup)", "-LabDuration=$($s.Measure)")
    if ($s.Serial) { $a += @($Doc.serial_trace_args) }
    if ($s.Trace) { $a += @(([string]$Doc.trace_args), "-tracefile=`"$ProjectDir\Saved\Traces\$label.utrace`"") }
    return $a
}

$Exe = if ($ExePath) { $ExePath } else { "$ProjectDir\$(($Doc.exe) -replace '/', '\')" }
$PkgRoot = if ($PkgRootOverride) { $PkgRootOverride } else { "$ProjectDir\Packaged\Test\Windows\HordeOptLab" }
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"
$ClockCsv = "$BatchDir\clock.csv"
$BatchJson = "$BatchDir\batch.json"
$CommitFile = "$ProjectDir\Packaged\Test\commit.txt"
$Commit = if (Test-Path $CommitFile) { ((Get-Content $CommitFile -Encoding UTF8 -TotalCount 1) -replace "^﻿", "").Trim() } else { "unknown" }
$Build12 = if ($Commit -match "^[0-9a-f]{12}") { $Commit.Substring(0, 12) } else { "" }

# ---- 예행 출력 ----
if ($DryRun) {
    $groupsShown = @($All | ForEach-Object { $_.Group } | Select-Object -Unique)
    $upper = 0.0
    foreach ($j in $All) { $upper += (Run-Sec $j.Slot) }
    Write-Host "DRYRUN batch=$Batch order=$Order groups=$($GroupNames -join '/') pick='$Pick' nfilter='$NFilter' build=$Build12 ($Commit)"
    Write-Host "exe: $Exe (exists=$(Test-Path $Exe))"
    foreach ($gn in $groupsShown) {
        $jobs = @($All | Where-Object { $_.Group -eq $gn })
        $slotsK = @($jobs | ForEach-Object { Slot-Key $_ } | Sort-Object -Unique).Count
        $sec = 0.0; foreach ($j in $jobs) { $sec += (Run-Sec $j.Slot) }
        Write-Host ("  {0,-9} slots={1,3} runs={2,3} est={3:N2} h (upper bound)" -f $gn, $slotsK, $jobs.Count, ($sec / 3600.0))
    }
    $R0 = if ($SkipR0 -or $Main.Count -eq 0) { 0 } else { 1 }
    $r0sec = if ($R0 -eq 1) { Run-Sec $Main[0].Slot } else { 0 }
    $totalSec = $upper + $r0sec
    $obsSec = $totalSec - $ObservedSavingSec * ($All.Count + $R0)
    Write-Host ("TOTAL slots={0} runs={1} (+{2} discarded r0) est={3:N2} h upper bound (startup {4} + warmup + measure + flush {5} s per run), {6:N2} h at the observed run length (-{7} s per run). Retry allowance not included (+5% = {8:N2} h)" -f $SlotCount, $All.Count, $R0, ($totalSec / 3600.0), $StartupSec, $FlushSec, ($obsSec / 3600.0), $ObservedSavingSec, ($obsSec * 1.05 / 3600.0))
    $lab = New-Object System.Collections.ArrayList
    if ($R0 -eq 1) { [void]$lab.Add((New-Job $Main[0].Slot 0).Label) }
    foreach ($j in $All) { [void]$lab.Add($j.Label) }
    Write-Host "CONDITIONS (one command per slot, run 1; the label of every run follows the same pattern):"
    $seen = @{}
    foreach ($j in $All) {
        $k = Slot-Key $j
        if ($seen.ContainsKey($k)) { continue }
        $seen[$k] = $true
        Write-Host "  [$($j.Group)] $($j.Label)"
        Write-Host "      `"$Exe`" $((Get-GameArgs $j $j.Label) -join ' ')"
    }
    Write-Host "RUN ORDER ($($lab.Count) runs including r0):"
    Write-Host ("  " + ($lab -join "`n  "))
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) "lab-dryrun-labels.txt"
    Set-Content -Path $tmp -Value $lab -Encoding UTF8
    $chk = (& py -3 -I "$PSScriptRoot\ablation-check.py" --labels $tmp 2>&1 | Out-String)
    Write-Host $chk.TrimEnd()
    $ok = ($LASTEXITCODE -eq 0)
    Remove-Item $tmp -ErrorAction SilentlyContinue
    Write-Host "  [check] py -3 -I Scripts/ablation-check.py <csv> --build $Build12"
    if (-not $ok) { Write-Host "DRYRUN FAIL"; exit 1 }
    exit 0
}

# ---- 시작 검사 ----
if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe. Run Scripts/package.ps1 first."; exit 1 }
$Blockers = @("UnrealEditor", "UnrealEditor-Cmd", "HordeOptLab-Win64-Test", "ShaderCompileWorker", "UnrealBuildTool", "chrome", "msedge", "firefox", "ffmpeg")
$Running = @(Get-Process -Name $Blockers -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0 -and -not $NoProcCheck) { Write-Host "FAIL: other programs are up ($(($Running | ForEach-Object { "$($_.ProcessName) pid $($_.Id)" }) -join ', ')). Close them first (chrome causes 7s periodic spikes)."; exit 1 }
if ($All.Count -eq 0) { Write-Host "FAIL: no runs selected."; exit 1 }
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
# 게임 창을 앞에 둔다. 1) MainWindowHandle을 최대 30초 기다린다 2) SetForegroundWindow 3) AppActivate(PID) 4) Alt 키 눌림 뒤 SetForegroundWindow
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

New-Item -ItemType Directory -Force -Path $OutCsv, $BatchDir, "$ProjectDir\Saved\Traces" | Out-Null

# ---- 진행 기록 ----
$Status = @{}   # 라벨 -> OK|FAIL|DISCARD
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
    Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},{2},{3},P1,{4},{5}" -f (Now), $Label, "$($j.Slot.Speed)-$($j.Slot.Cfg.name)", $j.Slot.N, $j.Group, (Clean $Reason))
}

# 시작 때 batch.json의 시작 시각과 해시를 적는다. 끝에서 다시 쓴다
$Started = Now
$Resumes = 0
if ($Resume -and (Test-Path $BatchJson)) {
    try { $old = Get-Content $BatchJson -Raw -Encoding UTF8 | ConvertFrom-Json; $Started = $old.started; $Resumes = [int]$old.resumes + 1 } catch { }
}
$PowerPlan = try { ((& powercfg /getactivescheme 2>&1 | Out-String).Trim() -replace "^.*:\s*", "") } catch { "" }

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
        $GameArgs = @($ExeArgsPrefix.Split(" ", [StringSplitOptions]::RemoveEmptyEntries)) + (Get-GameArgs $j $Label)
        Write-Host "RUN $Label [$($j.Group)] $(Get-Date -Format 'HH:mm:ss') cpu_busy_before=$busy"
        $Proc = Start-Process -FilePath $Exe -ArgumentList $GameArgs -PassThru
        $Focus = "focus=skipped"
        $Samples = New-Object System.Collections.ArrayList
        if (-not $NoFgCheck) { $Focus = Set-GameForeground $Proc; Write-Host $Focus }
        $TimeoutSec = $TimeoutFactor * (Run-Sec $j.Slot)
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
            # 카운터가 가끔 0을 돌려준다(V2W12Dry에서 한 표본). 0은 뺀다.
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
        # 메타데이터 대조(패키지 CSV에서). 틀리면 실행 실패
        $ck = @("-3", "-I", "$ProjectDir\Scripts\ablation-check.py", $pkgCsvFile)
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

function Retry-Label($j) { return "$($j.Slot.Prefix)-$($j.Slot.Speed)-$($j.Slot.Cfg.name)-N$($j.Slot.N)-P1-r$($j.RetryRun)" }
function Save-BatchJson($Ended, $State) {
    $ok = @($All | Where-Object { $Status[$_.Label] -eq "OK" -or $Status[(Retry-Label $_)] -eq "OK" }).Count
    $failRuns = @($Status.Values | Where-Object { $_ -eq "FAIL" }).Count
    $obj = [ordered]@{
        batch = $Batch; state = $State; commit = $Commit; started = $Started; ended = $Ended; resumes = $Resumes
        order = $Order; runs_override = $Runs; target_n = [int]$Sc.target_n; only = ($GroupNames -join ","); pick = $Pick; n_filter = $NFilter
        orbit = "r$($Sc.radius_m)/h$($Sc.height_m)"; measure_sec = [int]$Sc.measure_sec; power_plan = $PowerPlan; base_mhz = $BaseMhz
        conditions = $SlotCount; planned_runs = $All.Count; planned_runs_ok = $ok; planned_runs_missing = ($All.Count - $ok); failed_runs = $failRuns
        giveups = @($All | Where-Object { $Status[$_.Label] -eq "FAIL" -and $Status[(Retry-Label $_)] -eq "FAIL" } | ForEach-Object { $_.Label })
    }
    $obj | ConvertTo-Json -Depth 4 | Set-Content -Path $BatchJson -Encoding UTF8
}

Save-BatchJson "" "running"
$estH = [math]::Round((($All | ForEach-Object { Run-Sec $_.Slot } | Measure-Object -Sum).Sum) / 3600.0, 2)
Write-Host "BATCH $Batch commit=$Commit runs=$($All.Count) slots=$SlotCount order=$Order est<=${estH} h start=$(Now)"

# 묶음 맨 첫 실행 r0만 버린다(드라이버 PSO 캐시). 다시 시작하면 이미 있으면 건너뛴다
if (-not $SkipR0 -and $Main.Count -gt 0) {
    $j0 = New-Job $Main[0].Slot 0
    if (-not $Status.ContainsKey($j0.Label) -and $Status.Count -eq 0) {
        $r = Invoke-Run $j0 $j0.Label
        Write-Progress-Line $j0.Label "DISCARD" $r.P50 $r.P95 ("r0 discarded ok=" + $r.Ok + " " + $r.Reason)
        Write-Host "R0 discarded ($($j0.Label)) ok=$($r.Ok)"
    }
}

foreach ($j in $All) {
    if ($Status.ContainsKey($j.Label)) { Write-Host "SKIP $($j.Label) ($($Status[$j.Label]))"; continue }
    if (Test-Path "$OutCsv\$($j.Label).csv") {
        # -Resume 중 CSV는 옮겨졌는데 progress.log에 못 적은 경우. 대조만 다시 한다
        $ck = @("-3", "-I", "$ProjectDir\Scripts\ablation-check.py", "$OutCsv\$($j.Label).csv"); if ($Build12) { $ck += @("--build", $Build12) }
        $out = (& py @ck 2>&1 | Out-String)
        $m = [regex]::Match($out, "frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
        if ($LASTEXITCODE -eq 0) { Write-Progress-Line $j.Label "OK" $m.Groups[1].Value $m.Groups[2].Value "recovered csv"; Write-Host "RECOVERED $($j.Label)"; continue }
        New-Item -ItemType Directory -Force -Path "$OutCsv\failed" | Out-Null
        Move-Item "$OutCsv\$($j.Label).csv" "$OutCsv\failed\$($j.Label)-$(Get-Date -Format 'yyyyMMddHHmmss').csv" -Force
        Write-Progress-Line $j.Label "FAIL" "" "" "orphan csv failed check"; Write-Failure $j $j.Label "orphan csv failed check"; continue
    }
    [void](Run-Job $j $j.Label)
}

# 실패분만 새 회차 번호로 한 번 다시 잰다. 다시 실패하면 포기
$Retry = @($All | Where-Object { $Status[$_.Label] -eq "FAIL" })
if ($Retry.Count -gt 0) { Write-Host "RETRY PASS: $($Retry.Count) failed runs" }
foreach ($j in $Retry) {
    $rl = Retry-Label $j
    if ($Status.ContainsKey($rl)) { continue }
    $jr = New-Job $j.Slot $j.RetryRun
    if (-not (Run-Job $jr $jr.Label)) { Write-Host "GIVEUP $($jr.Label) (failed twice: $($j.Label))" }
}

$Ended = Now
Save-BatchJson $Ended "done"
$okCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "OK" }).Count
$failCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "FAIL" }).Count
Write-Host "DONE $Batch ok=$okCount fail=$failCount ended=$Ended (details: $BatchJson)"
& py -3 -I "$PSScriptRoot\summarize.py" --batch $Batch 2>&1 | Out-Host
$eq = "$PSScriptRoot\equivalence-check.py"
if (Test-Path $eq) { & py -3 -I $eq --batch $Batch 2>&1 | Out-Host }
exit 0
