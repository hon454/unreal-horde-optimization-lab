# 시험 실행 묶음(Spike<n>) 러너(ADR-0017, Docs/Worklog/V2.md "시험 실행"). 떠다니는 오브의 궤도 속도 × 구성 둘을 Test 패키지에서 잰다.
#   구성 final: 최종 구성(-LabStage=M5 -LabVariant=q4final). 구성 nofix: 고정 목표용 기법 셋을 끈 것(+ -LabSimLod=0 -LabFixedBounds=0 -LabCrowdFx=0, 빼 보기 A8-simlod·A8-bounds·A8-crowdfx의 인자).
#   모든 조건이 같은 궤도 반지름·높이, 따라가는 카메라(-LabPreset=3), 기준 장치(-LabMotionAids=1)를 쓴다. 속도 0이 정지 조건이다.
# 라벨: <Batch>-V<속도 번호>-<구성>-N<N>-P3-r<i>. 속도 번호는 -Speeds의 순서다(0부터). summarize.py가 조건 "V<번호>-<구성>"으로 요약한다.
# 순서: 묶음 맨 앞에 r0 하나(버림), 그 뒤 회차가 바깥 루프다(홀수 회차는 조건 순서대로, 짝수 회차는 거꾸로).
# 실행 절차(앞 창 검사, 실패 CSV 이동, progress.log, failures.csv)는 run-pool.ps1에서 옮겼다. 메타데이터는 spike-check.py가 대조한다.
# 총괄 묶음의 조건 표(full-conditions.json)는 exe 경로와 공통 인자만 읽고 고치지 않는다.
param(
    [string]$Batch = "Spike1",
    [int]$N = 10000,
    [Parameter(Mandatory = $true)][string]$RadiusM,
    [Parameter(Mandatory = $true)][string]$HeightM,
    # 쉼표로 이은 궤도 속도(m/s). 예: "0,1.5,3,6". -File로 부를 때 배열이 한 문자열로 넘어오므로 문자열로 받는다
    [Parameter(Mandatory = $true)][string]$Speeds,
    [int]$Runs = 5,
    [int]$DurationSec = 60,
    [int]$StartupSec = 30,
    [switch]$SkipR0,
    [switch]$DryRun
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"

$TimeoutSec = 3 * ($StartupSec + 15 + $DurationSec + 2)
$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Exe = "$ProjectDir\$(($Doc.exe) -replace '/', '\')"
$PkgRoot = "$ProjectDir\Packaged\Test\Windows\HordeOptLab"
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"

$Configs = @(
    @{ Name = "final"; Args = @("-LabStage=M5", "-LabVariant=q4final"); Expect = @("LabTechSimLod=1", "LabTechFixedBounds=1", "LabTechCrowdFx=1") },
    @{ Name = "nofix"; Args = @("-LabStage=M5", "-LabVariant=q4final", "-LabSimLod=0", "-LabFixedBounds=0", "-LabCrowdFx=0"); Expect = @("LabTechSimLod=0", "LabTechFixedBounds=0", "LabTechCrowdFx=0") }
)
$SpeedList = @($Speeds -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$Conds = @()
for ($i = 0; $i -lt $SpeedList.Count; $i++) {
    foreach ($c in $Configs) { $Conds += @{ Step = "V$i"; Speed = $SpeedList[$i]; Config = $c } }
}
function Label-Of($c, $run) { return "$Batch-$($c.Step)-$($c.Config.Name)-N$N-P3-r$run" }
function Get-GameArgs($c, $label) {
    $a = @($Doc.common_args) + @($c.Config.Args) + @("-LabN=$N", "-LabPreset=3", "-LabLabel=$label",
        "-LabOrbOrbitRadius=$RadiusM", "-LabOrbOrbitSpeed=$($c.Speed)", "-LabOrbHeight=$HeightM", "-LabMotionAids=1")
    if ($DurationSec -ne 60) { $a += "-LabDuration=$DurationSec" }
    return $a
}
function Get-Expect($c, $label) {
    return @("LabOrbit=r$RadiusM/v$($c.Speed)/h$HeightM/aids1", "LabPreset=P3", "LabN=$N", "LabLabel=$label", "LabAlive=$N", "LabOk=1",
        "LabStage=M5", "LabVariant=q4final", "LabTypes=2", "LabTypesAssets=ok", "LabHud=0", "LabShotAt=0") + @($c.Config.Expect)
}

# 실행 목록: r0(첫 조건, 버림) + 회차마다 전 조건
$Jobs = @()
if (-not $SkipR0) { $Jobs += @{ Cond = $Conds[0]; Run = 0 } }
for ($r = 1; $r -le $Runs; $r++) {
    $order = if ($r % 2 -eq 1) { $Conds } else { $o = @($Conds); [array]::Reverse($o); $o }
    foreach ($c in $order) { $Jobs += @{ Cond = $c; Run = $r } }
}

if ($DryRun) {
    $est = [math]::Round($Jobs.Count * ($StartupSec + 15 + $DurationSec + 2) / 60.0, 1)
    Write-Host "DRYRUN batch=$Batch N=$N radius=${RadiusM}m height=${HeightM}m speeds=$($SpeedList -join '/') conds=$($Conds.Count) runs=$($Jobs.Count) est=${est} min"
    foreach ($s in $SpeedList) {
        if ([double]$s -gt 0) { $T = 2 * [math]::PI * [double]$RadiusM / [double]$s; Write-Host ("  speed {0} m/s: period {1:N2} s, laps in {2} s = {3:N3}" -f $s, $T, $DurationSec, ($DurationSec / $T)) }
    }
    foreach ($j in $Jobs) { $l = Label-Of $j.Cond $j.Run; Write-Host "  $l" }
    Write-Host "  example: `"$Exe`" $((Get-GameArgs $Jobs[0].Cond (Label-Of $Jobs[0].Cond $Jobs[0].Run)) -join ' ')"
    exit 0
}

if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe"; exit 1 }
$Running = @(Get-Process UnrealEditor, HordeOptLab-Win64-Test, chrome -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) { Write-Host "FAIL: editor, game or chrome is up ($($Running.ProcessName -join ', ')). Close them first."; exit 1 }
if (Test-Path $ProgressLog) { Write-Host "FAIL: $ProgressLog exists. Use a new -Batch name (labels are never reused)."; exit 1 }
foreach ($j in $Jobs) { $l = Label-Of $j.Cond $j.Run; if (Test-Path "$OutCsv\$l.csv") { Write-Host "FAIL: CSV already exists for $l"; exit 1 } }
New-Item -ItemType Directory -Force -Path $OutCsv, $BatchDir | Out-Null
Set-Content -Path $FailuresCsv -Value "time,label,cond,n,preset,kind,reason" -Encoding UTF8
$Commit = if (Test-Path "$ProjectDir\Packaged\Test\commit.txt") { [string](Get-Content "$ProjectDir\Packaged\Test\commit.txt" -TotalCount 1) } else { "" }

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

function Invoke-Run($c, $run) {
    $Label = Label-Of $c $run
    $pkgCsvFile = "$PkgCsv\$Label.csv"
    $summary = "$PkgRoot\Saved\Lab\$Label.summary.txt"
    Remove-Item $pkgCsvFile, $summary -ErrorAction SilentlyContinue
    Write-Host "RUN $Label $(Now)"
    $Proc = Start-Process -FilePath $Exe -ArgumentList (Get-GameArgs $c $Label) -PassThru
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
            $out = (& py -3 -I "$ProjectDir\Scripts\spike-check.py" $pkgCsvFile @(Get-Expect $c $Label) 2>&1 | Out-String)
            $ckExit = $LASTEXITCODE
            Write-Host $out.TrimEnd()
            $m = [regex]::Match($out, "frames=(\d+) frame_p50=([0-9.]+) frame_p95=([0-9.]+)")
            if ($m.Success) { $p50 = $m.Groups[2].Value; $p95 = $m.Groups[3].Value }
            if ($ckExit -ne 0) { & $moveFail "$OutCsv\failed"; $reason = "metadata check failed" }
            elseif (-not $m.Success -or [int]$m.Groups[1].Value -lt 100) { & $moveFail "$OutCsv\failed"; $reason = "too few frames" }
            else { Move-Item $pkgCsvFile "$OutCsv\$Label.csv" }
        }
    }
    $st = if ($reason) { "FAIL" } elseif ($run -eq 0) { "DISCARD" } else { "OK" }
    Add-Content -Path $ProgressLog -Encoding UTF8 -Value ("{0}`t{1}`t{2}`t{3}`t{4}`t{5}" -f (Now), $Label, $st, $p50, $p95, (Clean $reason))
    if ($reason) { Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},{2},{3},P3,spike,{4}" -f (Now), $Label, "$($c.Step)-$($c.Config.Name)", $N, (Clean $reason)) }
    Write-Host "$st $Label p50=$p50 p95=$p95 $reason"
    return (-not $reason)
}

$Start = Now
Write-Host "BATCH $Batch start=$Start runs=$($Jobs.Count) build=$Commit"
$Retry = @()
$Ok = 0; $Fail = 0
foreach ($j in $Jobs) {
    if (Invoke-Run $j.Cond $j.Run) { $Ok++ } else { $Fail++; if ($j.Run -gt 0) { $Retry += $j } }
}
# 실패한 실행은 묶음 끝에서 새 회차 번호(회차 + Runs)로 한 번만 다시 잰다
$GaveUp = 0
foreach ($j in $Retry) {
    if (Invoke-Run $j.Cond ($j.Run + $Runs)) { $Ok++ } else { $GaveUp++; Write-Host "GIVEUP $(Label-Of $j.Cond ($j.Run + $Runs))" }
}
$Info = [ordered]@{ batch = $Batch; build = $Commit; start = $Start; end = (Now); n = $N; preset = "P3"; radius_m = $RadiusM; height_m = $HeightM; speeds = $SpeedList;
    duration_sec = $DurationSec; runs = $Runs; planned = $Jobs.Count; ok = $Ok; failed = $Fail; gave_up = $GaveUp }
$Info | ConvertTo-Json | Set-Content -Path "$BatchDir\batch.json" -Encoding UTF8
Write-Host "DONE $Batch ended=$(Now) ok=$Ok failed=$Fail gave_up=$GaveUp"
exit 0
