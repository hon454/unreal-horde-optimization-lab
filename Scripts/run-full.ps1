# 총괄 묶음(Full1) 러너(Docs/Worklog/Q5.md "2단위", plan.md "총괄 묶음 러너"). Test 패키지 exe를 직접 부른다. run-bench.ps1은 고치지 않고 그 절차를 옮겼다.
# 조건과 기대값은 Scripts/full-conditions.json이 정한다(조건 이름을 이 스크립트에 박지 않는다). 그룹: ladder, ablation, sub는 JSON의 group, maxn·actor는 사다리 조건의 N 변형, aa는 L8 기준점 반복, trace는 사다리 8단계 x 병렬·직렬.
# 라벨: <Batch>-<단계>-<변형>-N<N>-P<프리셋>-r<회차>. 트레이스는 <Batch>trace-... / <Batch>traceser-..., A/A는 같은 라벨 형식에 회차 11~13(요약이 -aa 조건으로 따로 쓴다), 실패 재측정은 회차 + Runs(A/A는 14~16).
# 실행 순서: 회차가 바깥 루프(r1 전 조건 -> r2 -> r3), 짝수 회차는 조건 목록을 뒤집는다(표류가 조건에 고르게 퍼지게). -Order cond면 조건 단위(같은 조건 3회 연달아).
# 실행마다 끝난 직후 ablation-check.py로 메타데이터를 대조한다. 실패(종료 코드, CSV 없음, 앞 창이 게임이 아님, 대조 틀림, 시간 초과)는 Measurements/<Batch>/failures.csv에 적고 넘어간다.
# 묶음 끝에서 실패분만 새 회차 번호로 한 번 다시 잰다. 다시 재도 실패하면 포기한다. 실패 CSV는 Saved/Profiling/CSV/failed/ 또는 fg-failed/로 옮겨 요약에서 빠진다.
# 진행은 Measurements/<Batch>/progress.log(탭 구분: 시각, 라벨, OK|FAIL|DISCARD, 프레임 p50, p95, 이유). -Resume이면 거기 있는 라벨은 건너뛴다.
param(
    [string]$Batch = "Full1",
    [ValidateSet("ladder", "ablation", "maxn", "actor", "sub", "aa", "trace")][string[]]$Only = @(),
    [int]$Runs = 3,
    # 목표 총수와 보조 묶음의 N 목록(예행에서 줄일 수 있다)
    [int]$TargetN = 10000,
    [string]$MaxNList = "5000,20000,30000",
    [string]$ActorNList = "500,1000,2000",
    # 측정 구간 초를 -LabDuration으로 바꾼다(예행 전용). 워밍업 15초는 게임 코드의 FLabBench::WarmupSec라 줄일 수 없다. 0이면 게임 기본 60초
    [int]$DurationOverride = 0,
    [switch]$Resume,
    [switch]$DryRun,
    [switch]$SkipR0,
    [switch]$NoFgCheck,
    [ValidateSet("round", "cond")][string]$Order = "round",
    # 예상 시간 계산용: 프로세스 시작부터 워밍업 시작까지(초)와 시간 제한 배수
    [int]$StartupSec = 30,
    [int]$TimeoutFactor = 3,
    # 러너 시험용(게임 없이 파싱·재시도·이어 하기 점검): 다른 실행 파일과 앞 인자(공백으로 나눈 한 문자열), 패키지 폴더. 측정에는 쓰지 않는다
    [string]$ExePath = "",
    [string]$ExeArgsPrefix = "",
    [string]$PkgRootOverride = "",
    [switch]$NoProcCheck
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"
$PSNativeCommandUseErrorActionPreference = $false

$WarmupSec = 15
$FlushSec = 2
$MeasureSec = if ($DurationOverride -gt 0) { $DurationOverride } else { 60 }
$RunSec = $StartupSec + $WarmupSec + $MeasureSec + $FlushSec
$TimeoutSec = $TimeoutFactor * $RunSec

# ---- 조건 JSON에서 작업 목록을 만든다 ----
$CondFile = "$PSScriptRoot\full-conditions.json"
$Doc = Get-Content $CondFile -Raw -Encoding UTF8 | ConvertFrom-Json
$Conds = @($Doc.conditions)
$Ladder = @($Conds | Where-Object { $_.group -eq "ladder" } | Sort-Object { [int]$_.ladder })
$Ablation = @($Conds | Where-Object { $_.group -eq "ablation" })
$Sub = @($Conds | Where-Object { $_.group -eq "sub" })
$Base8 = $Ladder | Where-Object { $_.ladder -eq "8" } | Select-Object -First 1
$Step1 = $Ladder | Where-Object { $_.ladder -eq "1" } | Select-Object -First 1
if (-not $Base8 -or -not $Step1) { Write-Host "FAIL: full-conditions.json has no ladder 1/8 condition."; exit 1 }
$MaxNs = @($MaxNList.Split(",", [StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { [int]$_ })
$ActorNs = @($ActorNList.Split(",", [StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { [int]$_ })
function Want($k) { return ($Only.Count -eq 0 -or $Only -contains $k) }

# 슬롯 = 조건 x N x 프리셋. 같은 슬롯의 회차들이 한 조건의 반복이다
$Slots = New-Object System.Collections.ArrayList
function Add-Slot($Kind, $Cond, $N, $Preset, $Prefix, $RunBase, $RunCount, $Serial) {
    [void]$Slots.Add(@{ Kind = $Kind; Cond = $Cond; N = $N; Preset = $Preset; Prefix = $Prefix; RunBase = $RunBase; RunCount = $RunCount; Serial = $Serial })
}
if (Want "ladder") { foreach ($p in 1, 2) { foreach ($c in $Ladder) { Add-Slot "ladder" $c $TargetN $p $Batch 0 $Runs $false } } }
if (Want "ablation") { foreach ($c in $Ablation) { Add-Slot "ablation" $c $TargetN 1 $Batch 0 $Runs $false } }
if (Want "maxn") { foreach ($c in ($Ladder | Where-Object { [int]$_.ladder -ge 2 })) { foreach ($n in $MaxNs) { Add-Slot "maxn" $c $n 1 $Batch 0 $Runs $false } } }
if (Want "actor") { foreach ($n in $ActorNs) { Add-Slot "actor" $Step1 $n 1 $Batch 0 $Runs $false } }
if (Want "sub") { foreach ($c in $Sub) { Add-Slot "sub" $c $TargetN 1 $Batch 0 $Runs $false } }
$MainSlots = @($Slots)
$AaSlots = New-Object System.Collections.ArrayList
if (Want "aa") { [void]$AaSlots.Add(@{ Kind = "aa"; Cond = $Base8; N = $TargetN; Preset = 1; Prefix = $Batch; RunBase = 10; RunCount = $Runs; Serial = $false }) }
$TraceSlots = New-Object System.Collections.ArrayList
if (Want "trace") {
    foreach ($c in $Ladder) {
        [void]$TraceSlots.Add(@{ Kind = "trace"; Cond = $c; N = $TargetN; Preset = 1; Prefix = "${Batch}trace"; RunBase = 0; RunCount = 1; Serial = $false })
        [void]$TraceSlots.Add(@{ Kind = "trace"; Cond = $c; N = $TargetN; Preset = 1; Prefix = "${Batch}traceser"; RunBase = 0; RunCount = 1; Serial = $true })
    }
}

function New-Job($Slot, $Run) {
    $c = $Slot.Cond
    $label = "$($Slot.Prefix)-$($c.label_step)-$($c.label_variant)-N$($Slot.N)-P$($Slot.Preset)-r$Run"
    return @{ Kind = $Slot.Kind; Cond = $c; N = $Slot.N; Preset = $Slot.Preset; Run = $Run; Label = $label; Trace = ($Slot.Kind -eq "trace"); Serial = $Slot.Serial
              RetryRun = $Run + $Slot.RunCount; Slot = $Slot }
}
function Slot-Key($j) { return "$($j.Kind)|$($j.Cond.name)|$($j.N)|P$($j.Preset)|$($j.Slot.Prefix)" }

# 본 실행 목록(회차 바깥 루프. 짝수 회차는 슬롯 순서를 뒤집는다)
$Main = New-Object System.Collections.ArrayList
if ($Order -eq "round") {
    for ($r = 1; $r -le $Runs; $r++) {
        $list = if ($r % 2 -eq 0) { $r2 = @($MainSlots); [array]::Reverse($r2); $r2 } else { $MainSlots }
        foreach ($s in $list) { [void]$Main.Add((New-Job $s $r)) }
    }
} else {
    foreach ($s in $MainSlots) { for ($r = 1; $r -le $Runs; $r++) { [void]$Main.Add((New-Job $s $r)) } }
}
$Aa = New-Object System.Collections.ArrayList
foreach ($s in $AaSlots) { for ($r = 1; $r -le $Runs; $r++) { [void]$Aa.Add((New-Job $s (10 + $r))) } }
$Tr = New-Object System.Collections.ArrayList
foreach ($s in $TraceSlots) { [void]$Tr.Add((New-Job $s 1)) }
$All = @($Main) + @($Aa) + @($Tr)
$SlotCount = @($All | ForEach-Object { Slot-Key $_ } | Sort-Object -Unique).Count

# ---- 게임 인자 ----
function Get-GameArgs($j, $label) {
    $a = @($Doc.common_args)
    $a += @($j.Cond.args)
    $a += @("-LabN=$($j.N)", "-LabPreset=$($j.Preset)", "-LabLabel=$label")
    if ($j.Serial) { $a += @($Doc.serial_trace_args) }
    if ($j.Trace) { $a += @("-trace=cpu,gpu,frame,counters,mass", "-tracefile=`"$ProjectDir\Saved\Traces\$label.utrace`"") }
    if ($DurationOverride -gt 0) { $a += "-LabDuration=$DurationOverride" }
    return $a
}

$Exe = if ($ExePath) { $ExePath } else { "$ProjectDir\$(($Doc.exe) -replace '/', '\')" }
$PkgRoot = if ($PkgRootOverride) { $PkgRootOverride } else { "$ProjectDir\Packaged\Test\Windows\HordeOptLab" }
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"
$BatchJson = "$BatchDir\batch.json"

# ---- 예행 출력 ----
if ($DryRun) {
    Write-Host "DRYRUN batch=$Batch order=$Order runs=$Runs targetN=$TargetN measure=${MeasureSec}s run=${RunSec}s(startup $StartupSec + warmup $WarmupSec + measure $MeasureSec + flush $FlushSec) timeout=${TimeoutSec}s"
    Write-Host "exe: $Exe (exists=$(Test-Path $Exe))"
    foreach ($k in "ladder", "ablation", "maxn", "actor", "sub", "aa", "trace") {
        $jobs = @($All | Where-Object { $_.Kind -eq $k })
        if ($jobs.Count -eq 0) { continue }
        $slotsK = @($jobs | ForEach-Object { Slot-Key $_ } | Sort-Object -Unique).Count
        Write-Host ("  {0,-9} slots={1,3} runs={2,3} est={3:N1} h" -f $k, $slotsK, $jobs.Count, ($jobs.Count * $RunSec / 3600.0))
    }
    $R0 = if ($SkipR0 -or $Main.Count -eq 0) { 0 } else { 1 }
    $Total = ($All.Count + $R0) * $RunSec
    Write-Host ("TOTAL slots={0} runs={1} (+{2} discarded r0) est={3:N2} h (retry allowance not included; +5% = {4:N2} h)" -f $SlotCount, $All.Count, $R0, ($Total / 3600.0), ($Total * 1.05 / 3600.0))
    Write-Host "LABELS (execution order; r0 first):"
    if ($R0 -eq 1) { $j0 = $Main[0]; Write-Host "  $($j0.Kind)`t$($Batch)-$($j0.Cond.label_step)-$($j0.Cond.label_variant)-N$($j0.N)-P$($j0.Preset)-r0 (discard)" }
    foreach ($j in $All) { Write-Host "  $($j.Kind)`t$($j.Label)" }
    Write-Host "EXAMPLE COMMANDS (one per kind):"
    foreach ($k in "ladder", "ablation", "maxn", "actor", "sub", "aa", "trace") {
        $j = $All | Where-Object { $_.Kind -eq $k } | Select-Object -First 1
        if ($j) { Write-Host "  [$k] `"$Exe`" $((Get-GameArgs $j $j.Label) -join ' ')" }
    }
    Write-Host "  [check] py -3 -I Scripts/ablation-check.py <csv> --cond <name> [--serial]"
    exit 0
}

# ---- 시작 검사 ----
if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe. Run Scripts/package.ps1 first."; exit 1 }
$Running = @(Get-Process UnrealEditor, HordeOptLab-Win64-Test, chrome -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0 -and -not $NoProcCheck) { Write-Host "FAIL: editor, game or chrome is up ($($Running.ProcessName -join ', ') pid $($Running.Id -join ', ')). Close them first (chrome causes 7s periodic spikes)."; exit 1 }
if ($All.Count -eq 0) { Write-Host "FAIL: no runs selected."; exit 1 }

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
# run-bench.ps1의 Set-GameForeground와 같다
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

function Now { return (Get-Date -Format "yyyy-MM-dd HH:mm:ss") }
function Clean($s) { return (($s -replace "[\r\n]+", " ") -replace ",", ";").Trim() }
function Write-Progress-Line($Label, $St, $P50, $P95, $Reason) {
    $Status[$Label] = $St
    Add-Content -Path $ProgressLog -Encoding UTF8 -Value ("{0}`t{1}`t{2}`t{3}`t{4}`t{5}" -f (Now), $Label, $St, $P50, $P95, (Clean $Reason))
}
function Write-Failure($j, $Label, $Reason) {
    Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},{2},{3},P{4},{5},{6}" -f (Now), $Label, $j.Cond.name, $j.N, $j.Preset, $j.Kind, (Clean $Reason))
}

# 시작 때 batch.json의 시작 시각과 해시를 적는다. 끝에서 다시 쓴다
$CommitFile = "$ProjectDir\Packaged\Test\commit.txt"
$Commit = if (Test-Path $CommitFile) { ((Get-Content $CommitFile -Encoding UTF8 -TotalCount 1) -replace "^﻿", "").Trim() } else { "unknown" }
$Started = Now
$Resumes = 0
if ($Resume -and (Test-Path $BatchJson)) {
    try { $old = Get-Content $BatchJson -Raw -Encoding UTF8 | ConvertFrom-Json; $Started = $old.started; $Resumes = [int]$old.resumes + 1 } catch { }
}

# ---- 실행 하나 ----
function Invoke-Run($j, $Label) {
    $res = @{ Ok = $false; Reason = ""; P50 = ""; P95 = "" }
    $pkgCsvFile = "$PkgCsv\$Label.csv"
    $summary = "$PkgRoot\Saved\Lab\$Label.summary.txt"
    try {
        Remove-Item $pkgCsvFile, $summary -ErrorAction SilentlyContinue
        $GameArgs = @($ExeArgsPrefix.Split(" ", [StringSplitOptions]::RemoveEmptyEntries)) + (Get-GameArgs $j $Label)
        Write-Host "RUN $Label [$($j.Kind)] $(Get-Date -Format 'HH:mm:ss')"
        $Proc = Start-Process -FilePath $Exe -ArgumentList $GameArgs -PassThru
        $Focus = "focus=skipped"
        $Samples = New-Object System.Collections.ArrayList
        if (-not $NoFgCheck) { $Focus = Set-GameForeground $Proc; Write-Host $Focus }
        $Deadline = (Get-Date).AddSeconds($TimeoutSec)
        $TimedOut = $false
        while (-not $Proc.HasExited) {
            if ((Get-Date) -gt $Deadline) { $TimedOut = $true; Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue; break }
            if (-not $NoFgCheck) { [void]$Samples.Add(@{ T = (Get-Date); Pid = (Get-FgPid) }) }
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
        }
        if (Test-Path $summary) { Get-Content $summary | Select-Object -Last 4 | Out-Host }
        if ($FgFail) { & $moveFail "$OutCsv\fg-failed"; $res.Reason = "foreground not on game during measurement ($FgNote)"; return $res }
        if ($Exit -ne 0) { & $moveFail $failDir; $res.Reason = "exit code $Exit"; return $res }
        if (-not (Test-Path $pkgCsvFile)) { $res.Reason = "no csv in package Saved/Profiling/CSV"; return $res }
        # 메타데이터 대조(패키지 CSV에서). 틀리면 실행 실패
        $ck = @("-3", "-I", "$ProjectDir\Scripts\ablation-check.py", $pkgCsvFile, "--cond", $j.Cond.name)
        if ($j.Serial) { $ck += "--serial" }
        $out = (& py @ck 2>&1 | Out-String)
        $ckExit = $LASTEXITCODE
        Write-Host $out.TrimEnd()
        $m = [regex]::Match($out, "frames=(\d+) frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
        if ($m.Success) { $res.P50 = $m.Groups[2].Value; $res.P95 = $m.Groups[3].Value }
        if ($ckExit -ne 0) {
            & $moveFail $failDir
            $res.Reason = "metadata check failed: " + (([regex]::Match($out, "CHECK [^\r\n]*FAIL[^\r\n]*")).Value)
            return $res
        }
        if (-not $m.Success -or [int]$m.Groups[1].Value -lt 100) { & $moveFail $failDir; $res.Reason = "too few frames or unparsable check output"; return $res }
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

function Save-BatchJson($Ended, $State) {
    $ok = @($All | Where-Object { $Status[$_.Label] -eq "OK" -or $Status["$($_.Slot.Prefix)-$($_.Cond.label_step)-$($_.Cond.label_variant)-N$($_.N)-P$($_.Preset)-r$($_.RetryRun)"] -eq "OK" }).Count
    $failRuns = @($Status.Values | Where-Object { $_ -eq "FAIL" }).Count
    $obj = [ordered]@{
        batch = $Batch; state = $State; commit = $Commit; started = $Started; ended = $Ended; resumes = $Resumes
        order = $Order; runs_per_condition = $Runs; target_n = $TargetN; duration_override = $DurationOverride; only = ($Only -join ",")
        conditions = $SlotCount; planned_runs = $All.Count; planned_runs_ok = $ok; planned_runs_missing = ($All.Count - $ok); failed_runs = $failRuns
        giveups = @($All | Where-Object { $Status[$_.Label] -eq "FAIL" -and $Status["$($_.Slot.Prefix)-$($_.Cond.label_step)-$($_.Cond.label_variant)-N$($_.N)-P$($_.Preset)-r$($_.RetryRun)"] -eq "FAIL" } | ForEach-Object { $_.Label })
    }
    $obj | ConvertTo-Json -Depth 4 | Set-Content -Path $BatchJson -Encoding UTF8
}

Save-BatchJson "" "running"
Write-Host "BATCH $Batch commit=$Commit runs=$($All.Count) slots=$SlotCount order=$Order est=$([math]::Round(($All.Count + 1) * $RunSec / 3600.0, 2)) h start=$(Now)"

# 묶음 맨 첫 실행 r0만 버린다(드라이버 PSO 캐시). 다시 시작하면 이미 있으면 건너뛴다
if (-not $SkipR0 -and $Main.Count -gt 0) {
    $s0 = $Main[0].Slot
    $j0 = New-Job $s0 0
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
        $ck = @("-3", "-I", "$ProjectDir\Scripts\ablation-check.py", "$OutCsv\$($j.Label).csv", "--cond", $j.Cond.name); if ($j.Serial) { $ck += "--serial" }
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
    $rl = "$($j.Slot.Prefix)-$($j.Cond.label_step)-$($j.Cond.label_variant)-N$($j.N)-P$($j.Preset)-r$($j.RetryRun)"
    if ($Status.ContainsKey($rl)) { continue }
    $jr = New-Job $j.Slot $j.RetryRun
    if (-not (Run-Job $jr $jr.Label)) { Write-Host "GIVEUP $($jr.Label) (failed twice: $($j.Label))" }
}

$Ended = Now
Save-BatchJson $Ended "done"
$okCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "OK" }).Count
$failCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "FAIL" }).Count
Write-Host "DONE $Batch ok=$okCount fail=$failCount ended=$Ended (details: $BatchJson)"
$eq = "$PSScriptRoot\equivalence-check.py"
if (Test-Path $eq) { & py -3 -I $eq --batch $Batch 2>&1 | Out-Host }
exit 0
