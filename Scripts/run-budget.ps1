# 참고 묶음 Budget1 러너(Docs/Worklog/Q7.md "측정: Budget1"). 최종 구성 q4final(-LabStage=M5)에서 -LabBudgetMs만 바꿔 잰다.
# 예산 끔(0), 1, 2, 3, 4, 6ms x 호드 1만·2만 x 오브 시점(P1) x 3회. 조건 인자는 Scripts/full-conditions.json의 common_args와 L8-q4final 인자에 -LabBudgetMs=<x>를 더한 것이다.
# run-bench.ps1은 -Variant 허용 목록에 q4final이 없고 라벨에 조건을 담을 수 없어 부르지 않는다. run-pool.ps1·run-full.ps1처럼 Test 패키지 exe를 직접 부르고 그 실행 절차(앞 창 검사, 메타데이터 대조, 실패 CSV 이동)를 옮겼다.
# run-full.ps1, full-conditions.json, run-bench.ps1은 고치지 않는다.
# 코드 기본값은 53f1dfb 이후 4ms다. 그래서 예산 4는 LabOverrides `-`, 나머지 값은 `budget_ms=<x>`가 붙는다. 이 러너는 Budget1(기본값 2ms 때 잰 묶음)용이고 -LabBudgetMs를 늘 명시하므로 기본값과 무관하게 돈다.
# 라벨: <Batch>-B<예산>-q4final-N<N>-P1-r<회차>. 예산 0은 끔이다. summarize.py의 총괄 묶음 라벨 형식(단계 칸 B<예산>)을 따른다.
# 실행 순서(run-full.ps1과 같다): 회차가 바깥 루프(r1 전 조건 -> r2 -> r3), 짝수 회차는 조건 목록을 뒤집는다. 맨 앞에 r0 한 번(드라이버 PSO 캐시)을 돌려 버린다.
# 실행마다 Scripts/check-budget.py로 LabOk, CSV, 메타데이터(LabBudgetMs, LabOverrides, LabBudget)를 대조한다. 틀리면 실패다. 벤치 끝의 종료 코드는 늘 0이라 종료 코드는 판정에 약하다.
# 진행은 Measurements/<Batch>/progress.log(탭 구분), 실패는 failures.csv. 실패분은 묶음 끝에서 새 회차 번호(회차 + Runs)로 한 번 다시 잰다. -Resume이면 progress.log에 있는 라벨은 건너뛴다.
param(
    [string]$Batch = "Budget1",
    [string]$Budgets = "0,1,2,3,4,6",
    [string]$NList = "10000,20000",
    [int]$Runs = 3,
    [int]$StartupSec = 30,
    [int]$TimeoutFactor = 3,
    [switch]$Resume,
    [switch]$SkipR0,
    [switch]$DryRun
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"
$PSNativeCommandUseErrorActionPreference = $false

$WarmupSec = 15; $MeasureSec = 60; $FlushSec = 2
$RunSec = $StartupSec + $WarmupSec + $MeasureSec + $FlushSec
$TimeoutSec = $TimeoutFactor * $RunSec

$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Base = $Doc.conditions | Where-Object { $_.name -eq "L8-q4final" } | Select-Object -First 1
if (-not $Base) { Write-Host "FAIL: no L8-q4final in full-conditions.json"; exit 1 }
$Exe = "$ProjectDir\$(($Doc.exe) -replace '/', '\')"
$PkgRoot = "$ProjectDir\Packaged\Test\Windows\HordeOptLab"
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"

$BudgetList = @($Budgets.Split(",", [StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { [int]$_ })
$NValues = @($NList.Split(",", [StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { [int]$_ })
$Slots = @()
foreach ($n in $NValues) { foreach ($b in $BudgetList) { $Slots += , @{ Budget = $b; N = $n } } }

function New-Job($Slot, $Run) {
    $label = "$Batch-B$($Slot.Budget)-q4final-N$($Slot.N)-P1-r$Run"
    return @{ Budget = $Slot.Budget; N = $Slot.N; Run = $Run; Label = $label; Slot = $Slot; RetryRun = $Run + $Runs }
}
function Get-GameArgs($j) {
    return @($Doc.common_args) + @($Base.args) + @("-LabBudgetMs=$($j.Budget)", "-LabN=$($j.N)", "-LabPreset=1", "-LabLabel=$($j.Label)")
}

$Main = New-Object System.Collections.ArrayList
for ($r = 1; $r -le $Runs; $r++) {
    $list = if ($r % 2 -eq 0) { $t = @($Slots); [array]::Reverse($t); $t } else { $Slots }
    foreach ($s in $list) { [void]$Main.Add((New-Job $s $r)) }
}
$J0 = New-Job $Slots[0] 0

if ($DryRun) {
    $R0 = if ($SkipR0) { 0 } else { 1 }
    Write-Host ("DRYRUN batch=$Batch slots={0} runs={1} (+{2} discarded r0) run={3}s timeout={4}s est={5:N2} h (retry allowance not included)" -f $Slots.Count, $Main.Count, $R0, $RunSec, $TimeoutSec, (($Main.Count + $R0) * $RunSec / 3600.0))
    Write-Host "exe: $Exe (exists=$(Test-Path $Exe))"
    if (-not $SkipR0) { Write-Host "  r0 (discard)`t$($J0.Label)`t$((Get-GameArgs $J0) -join ' ')" }
    $i = 0
    foreach ($j in $Main) { $i++; Write-Host ("  {0,2}`t{1}`t{2}" -f $i, $j.Label, ((Get-GameArgs $j) -join ' ')) }
    Write-Host "  [check] py -3 -I Scripts/check-budget.py <csv> --budget <ms> --n <N> --label <label>"
    exit 0
}

if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe. Run Scripts/package.ps1 first."; exit 1 }
$Running = @(Get-Process UnrealEditor, HordeOptLab-Win64-Test, chrome -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) { Write-Host "FAIL: editor, game or chrome is up ($($Running.ProcessName -join ', ') pid $($Running.Id -join ', ')). Close them first."; exit 1 }

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
function Get-FgPid { $p = [uint32]0; [void][FgWin]::GetWindowThreadProcessId([FgWin]::GetForegroundWindow(), [ref]$p); return [int]$p }
function Set-GameForeground($Proc) {
    $Deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $Deadline) { $Proc.Refresh(); if ($Proc.HasExited) { return "focus=exited" }; if ($Proc.MainWindowHandle -ne [IntPtr]::Zero) { break }; Start-Sleep -Milliseconds 500 }
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
function Now { return (Get-Date -Format "yyyy-MM-dd HH:mm:ss") }
function Clean($s) { return (($s -replace "[\r\n]+", " ") -replace ",", ";").Trim() }

New-Item -ItemType Directory -Force -Path $OutCsv, $BatchDir | Out-Null
$Status = @{}
if (Test-Path $ProgressLog) {
    if (-not $Resume) { Write-Host "FAIL: $ProgressLog exists. Use -Resume to continue, or a new -Batch name (labels are never reused)."; exit 1 }
    foreach ($line in (Get-Content $ProgressLog -Encoding UTF8)) { $f = $line.Split("`t"); if ($f.Count -ge 3) { $Status[$f[1]] = $f[2] } }
} else {
    $dups = @($Main | Where-Object { Test-Path "$OutCsv\$($_.Label).csv" } | ForEach-Object { $_.Label })
    if ($dups.Count -gt 0) { Write-Host "FAIL: CSV already exists for $($dups.Count) labels, e.g. $($dups[0]). Use a new -Batch."; exit 1 }
}
if (-not (Test-Path $FailuresCsv)) { Set-Content -Path $FailuresCsv -Value "time,label,cond,n,preset,kind,reason" -Encoding UTF8 }

function Write-Progress-Line($Label, $St, $P50, $P95, $Reason) {
    $Status[$Label] = $St
    Add-Content -Path $ProgressLog -Encoding UTF8 -Value ("{0}`t{1}`t{2}`t{3}`t{4}`t{5}" -f (Now), $Label, $St, $P50, $P95, (Clean $Reason))
}

function Invoke-Run($j) {
    $Label = $j.Label
    $res = @{ Ok = $false; Reason = ""; P50 = ""; P95 = "" }
    $pkgCsvFile = "$PkgCsv\$Label.csv"
    $summary = "$PkgRoot\Saved\Lab\$Label.summary.txt"
    $Proc = $null
    try {
        Remove-Item $pkgCsvFile, $summary -ErrorAction SilentlyContinue
        Write-Host "RUN $Label $(Get-Date -Format 'HH:mm:ss')"
        $Proc = Start-Process -FilePath $Exe -ArgumentList (Get-GameArgs $j) -PassThru
        $Focus = Set-GameForeground $Proc
        Write-Host $Focus
        $Samples = New-Object System.Collections.ArrayList
        $Deadline = (Get-Date).AddSeconds($TimeoutSec)
        $TimedOut = $false
        while (-not $Proc.HasExited) {
            if ((Get-Date) -gt $Deadline) { $TimedOut = $true; Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue; break }
            [void]$Samples.Add(@{ T = (Get-Date); Pid = (Get-FgPid) })
            [void]$Proc.WaitForExit(2000)
        }
        $Proc.WaitForExit()
        $Exit = $Proc.ExitCode
        $moveFail = { param($dir) if (Test-Path $pkgCsvFile) { New-Item -ItemType Directory -Force -Path $dir | Out-Null; Move-Item $pkgCsvFile "$dir\$Label-$(Get-Date -Format 'yyyyMMddHHmmss').csv" -Force } }
        if ($TimedOut) { & $moveFail "$OutCsv\failed"; $res.Reason = "timeout ${TimeoutSec}s, process killed"; return $res }
        $FgFail = $false; $FgNote = ""
        if (Test-Path $summary) {
            $Text = Get-Content $summary -Raw
            $Sec = [double]([regex]::Match($Text, "measure_sec=([0-9.]+)").Groups[1].Value)
            $End = (Get-Item $summary).LastWriteTime
            $In = @($Samples | Where-Object { $_.T -ge $End.AddSeconds(-$Sec) -and $_.T -le $End })
            $Non = @($In | Where-Object { $_.Pid -ne $Proc.Id }).Count
            $FgNote = "fg=$($In.Count - $Non)/$($In.Count) $Focus"
            Add-Content -Path $summary -Value ("`n" + $FgNote)
            if ($In.Count -eq 0 -or $Non -gt 0) { $FgFail = $true }
            Get-Content $summary | Select-Object -Last 4 | Out-Host
        }
        if ($FgFail) { & $moveFail "$OutCsv\fg-failed"; $res.Reason = "foreground not on game during measurement ($FgNote)"; return $res }
        if ($Exit -ne 0) { & $moveFail "$OutCsv\failed"; $res.Reason = "exit code $Exit"; return $res }
        if (-not (Test-Path $pkgCsvFile)) { $res.Reason = "no csv in package Saved/Profiling/CSV"; return $res }
        $out = (& py -3 -I "$ProjectDir\Scripts\check-budget.py" $pkgCsvFile --budget $j.Budget --n $j.N --label $Label 2>&1 | Out-String)
        $ckExit = $LASTEXITCODE
        Write-Host $out.TrimEnd()
        $m = [regex]::Match($out, "frames=(\d+) frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
        if ($m.Success) { $res.P50 = $m.Groups[2].Value; $res.P95 = $m.Groups[3].Value }
        if ($ckExit -ne 0) { & $moveFail "$OutCsv\failed"; $res.Reason = "metadata check failed: " + (([regex]::Match($out, "CHECK [^\r\n]*FAIL[^\r\n]*")).Value); return $res }
        if (-not $m.Success -or [int]$m.Groups[1].Value -lt 100) { & $moveFail "$OutCsv\failed"; $res.Reason = "too few frames or unparsable check output"; return $res }
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

function Run-Job($j) {
    $r = Invoke-Run $j
    if ($r.Ok) { Write-Progress-Line $j.Label "OK" $r.P50 $r.P95 ""; Write-Host "OK $($j.Label) p50=$($r.P50) p95=$($r.P95)" }
    else {
        Write-Progress-Line $j.Label "FAIL" $r.P50 $r.P95 $r.Reason
        Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},B{2}-q4final,{3},P1,budget,{4}" -f (Now), $j.Label, $j.Budget, $j.N, (Clean $r.Reason))
        Write-Host "FAIL $($j.Label) : $($r.Reason)"
    }
    return $r.Ok
}

Write-Host "BATCH $Batch runs=$($Main.Count) slots=$($Slots.Count) est=$([math]::Round(($Main.Count + 1) * $RunSec / 3600.0, 2)) h start=$(Now)"
if (-not $SkipR0 -and $Status.Count -eq 0) {
    $r = Invoke-Run $J0
    Write-Progress-Line $J0.Label "DISCARD" $r.P50 $r.P95 ("r0 discarded ok=" + $r.Ok + " " + $r.Reason)
    Write-Host "R0 discarded ($($J0.Label)) ok=$($r.Ok)"
    # r0 CSV는 요약에서 빠진다(summarize.py가 r0을 뺀다). 패키지에서 가져오지 않았다면 그대로 둔다
}
foreach ($j in $Main) {
    if ($Status.ContainsKey($j.Label)) { Write-Host "SKIP $($j.Label) ($($Status[$j.Label]))"; continue }
    [void](Run-Job $j)
}
$Retry = @($Main | Where-Object { $Status[$_.Label] -eq "FAIL" })
if ($Retry.Count -gt 0) { Write-Host "RETRY PASS: $($Retry.Count) failed runs" }
foreach ($j in $Retry) {
    $jr = New-Job $j.Slot $j.RetryRun
    if ($Status.ContainsKey($jr.Label)) { continue }
    if (-not (Run-Job $jr)) { Write-Host "GIVEUP $($jr.Label) (failed twice: $($j.Label))" }
}
$okCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "OK" }).Count
$failCount = @($Status.GetEnumerator() | Where-Object { $_.Value -eq "FAIL" }).Count
Write-Host "DONE $Batch ok=$okCount fail=$failCount ended=$(Now)"
exit 0
