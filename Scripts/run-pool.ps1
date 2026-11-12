# 엔티티 풀 묶음(Pool<n>) 러너(Docs/Worklog/Q6.md "Full2 재측정"). 최종 구성 1만 P1을 긴 측정 구간으로 두 번 돌린다: 풀 켬(L8-q4final), 풀 끔(A8-pool, -LabEntityPool=0).
# 시간 추이는 프레임 CSV의 Lab/IskmTracks, GPUMem/LocalUsedMB로 본다(Scripts/pool-timeline.py). M5 E5 (b)와 같은 방법이다(Docs/Worklog/M5.md).
# 조건 인자와 기대 메타데이터는 Scripts/full-conditions.json에서 읽는다. run-full.ps1의 실행 절차(앞 창 검사, 메타데이터 대조, 실패 CSV 이동)를 옮겼다. run-full.ps1과 run-bench.ps1은 고치지 않는다.
# 라벨: <Batch>-<단계>-<변형>-N<N>-P1-r<i>. 실패한 실행은 Measurements/<Batch>/failures.csv에 적고 새 회차 번호로 한 번만 다시 잰다.
param(
    [string]$Batch = "Pool1",
    [int]$N = 10000,
    [int]$DurationSec = 960,
    [string[]]$Conds = @("L8-q4final", "A8-pool"),
    [int]$StartupSec = 30,
    [switch]$DryRun
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"

$TimeoutSec = 2 * ($StartupSec + 15 + $DurationSec + 2)
$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Exe = "$ProjectDir\$(($Doc.exe) -replace '/', '\')"
$PkgRoot = "$ProjectDir\Packaged\Test\Windows\HordeOptLab"
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"

$Jobs = @()
foreach ($name in $Conds) {
    $c = $Doc.conditions | Where-Object { $_.name -eq $name } | Select-Object -First 1
    if (-not $c) { Write-Host "FAIL: no condition $name in full-conditions.json"; exit 1 }
    $Jobs += @{ Cond = $c; Run = 1 }
}
function Label-Of($j) { return "$Batch-$($j.Cond.label_step)-$($j.Cond.label_variant)-N$N-P1-r$($j.Run)" }
function Get-GameArgs($j, $label) {
    return @($Doc.common_args) + @($j.Cond.args) + @("-LabN=$N", "-LabPreset=1", "-LabLabel=$label", "-LabDuration=$DurationSec")
}

if ($DryRun) {
    Write-Host "DRYRUN batch=$Batch N=$N duration=${DurationSec}s timeout=${TimeoutSec}s est=$([math]::Round($Jobs.Count * ($StartupSec + 15 + $DurationSec + 2) / 60.0, 1)) min"
    foreach ($j in $Jobs) { $l = Label-Of $j; Write-Host "  `"$Exe`" $((Get-GameArgs $j $l) -join ' ')" }
    exit 0
}

if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe"; exit 1 }
$Running = @(Get-Process UnrealEditor, HordeOptLab-Win64-Test, chrome -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) { Write-Host "FAIL: editor, game or chrome is up ($($Running.ProcessName -join ', ')). Close them first."; exit 1 }
if (Test-Path $ProgressLog) { Write-Host "FAIL: $ProgressLog exists. Use a new -Batch name (labels are never reused)."; exit 1 }
foreach ($j in $Jobs) { if (Test-Path "$OutCsv\$(Label-Of $j).csv") { Write-Host "FAIL: CSV already exists for $(Label-Of $j)"; exit 1 } }
New-Item -ItemType Directory -Force -Path $OutCsv, $BatchDir | Out-Null
Set-Content -Path $FailuresCsv -Value "time,label,cond,n,preset,kind,reason" -Encoding UTF8

if (-not ('FgWin' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class FgWin {
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
}
function Get-FgPid { $p = [uint32]0; [void][FgWin]::GetWindowThreadProcessId([FgWin]::GetForegroundWindow(), [ref]$p); return [int]$p }
function Set-GameForeground($Proc) {
    $Deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $Deadline) { $Proc.Refresh(); if ($Proc.HasExited) { return "focus=exited" }; if ($Proc.MainWindowHandle -ne [IntPtr]::Zero) { break }; Start-Sleep -Milliseconds 500 }
    if ($Proc.MainWindowHandle -eq [IntPtr]::Zero) { return "focus=NOWINDOW" }
    [void][FgWin]::SetForegroundWindow($Proc.MainWindowHandle); Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=ok" }
    try { [void](New-Object -ComObject WScript.Shell).AppActivate($Proc.Id) } catch { }
    Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=appactivate" }
    return "focus=FAILED"
}
function Now { return (Get-Date -Format "yyyy-MM-dd HH:mm:ss") }
function Clean($s) { return (($s -replace "[\r\n]+", " ") -replace ",", ";").Trim() }

function Invoke-Run($j) {
    $Label = Label-Of $j
    $pkgCsvFile = "$PkgCsv\$Label.csv"
    $summary = "$PkgRoot\Saved\Lab\$Label.summary.txt"
    Remove-Item $pkgCsvFile, $summary -ErrorAction SilentlyContinue
    Write-Host "RUN $Label $(Now)"
    $Proc = Start-Process -FilePath $Exe -ArgumentList (Get-GameArgs $j $Label) -PassThru
    $Focus = Set-GameForeground $Proc
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
    $reason = ""; $p50 = ""; $p95 = ""
    if ($TimedOut) { & $moveFail "$OutCsv\failed"; $reason = "timeout ${TimeoutSec}s" }
    else {
        $FgNote = ""
        if (Test-Path $summary) {
            $Text = Get-Content $summary -Raw
            $Sec = [double]([regex]::Match($Text, "measure_sec=([0-9.]+)").Groups[1].Value)
            $End = (Get-Item $summary).LastWriteTime
            $In = @($Samples | Where-Object { $_.T -ge $End.AddSeconds(-$Sec) -and $_.T -le $End })
            $Non = @($In | Where-Object { $_.Pid -ne $Proc.Id }).Count
            $FgNote = "fg=$($In.Count - $Non)/$($In.Count) $Focus"
            Add-Content -Path $summary -Value ("`n" + $FgNote)
            if ($In.Count -eq 0 -or $Non -gt 0) { $reason = "foreground not on game ($FgNote)"; & $moveFail "$OutCsv\fg-failed" }
        }
        if (-not $reason -and $Exit -ne 0) { & $moveFail "$OutCsv\failed"; $reason = "exit code $Exit" }
        if (-not $reason -and -not (Test-Path $pkgCsvFile)) { $reason = "no csv" }
        if (-not $reason) {
            $out = (& py -3 -I "$ProjectDir\Scripts\ablation-check.py" $pkgCsvFile --cond $j.Cond.name 2>&1 | Out-String)
            $ckExit = $LASTEXITCODE
            Write-Host $out.TrimEnd()
            $m = [regex]::Match($out, "frames=(\d+) frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
            if ($m.Success) { $p50 = $m.Groups[2].Value; $p95 = $m.Groups[3].Value }
            if ($ckExit -ne 0) { & $moveFail "$OutCsv\failed"; $reason = "metadata check failed" }
            elseif (-not $m.Success -or [int]$m.Groups[1].Value -lt 100) { & $moveFail "$OutCsv\failed"; $reason = "too few frames" }
            else { Move-Item $pkgCsvFile "$OutCsv\$Label.csv" }
        }
    }
    $st = if ($reason) { "FAIL" } else { "OK" }
    Add-Content -Path $ProgressLog -Encoding UTF8 -Value ("{0}`t{1}`t{2}`t{3}`t{4}`t{5}" -f (Now), $Label, $st, $p50, $p95, (Clean $reason))
    if ($reason) { Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},{2},{3},P1,pool,{4}" -f (Now), $Label, $j.Cond.name, $N, (Clean $reason)) }
    Write-Host "$st $Label p50=$p50 p95=$p95 $reason"
    return (-not $reason)
}

Write-Host "BATCH $Batch start=$(Now) runs=$($Jobs.Count) duration=${DurationSec}s"
foreach ($j in $Jobs) {
    if (-not (Invoke-Run $j)) {
        $j2 = @{ Cond = $j.Cond; Run = $j.Run + 1 }
        if (-not (Invoke-Run $j2)) { Write-Host "GIVEUP $(Label-Of $j2)" }
    }
}
Write-Host "DONE $Batch ended=$(Now)"
exit 0
