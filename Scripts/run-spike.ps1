# 작은 묶음 러너(ADR-0017, Docs/Worklog/V2.md "시험 실행"). 떠다니는 오브의 궤도 속도 × 구성을 Test 패키지에서 잰다.
#   v2 W10부터 구성은 사다리 단계 s1~s8(-LabStage=1~8)과 단계 8의 빼 보기 a8*, 직렬 트레이스용 s<n>ser다(아래 $ConfigTable).
#   모든 조건이 같은 궤도 반지름·높이를 쓴다. 카메라는 P1(오브를 따라간다, v2 W1부터 기본)이다. 속도 0이 정지 조건이다.
#   v2 W1부터: -WarmupSec(기본 -1 = 게임의 기본값: 이동 60초, 정지 15초)과 -DurationSec을 늘 넘긴다. 그전 묶음(Spike1, Spike2)은 P3·15초 워밍업이었다.
# 라벨: <Batch>-V<속도 번호>-<구성>-N<N>-P1-r<i>. 속도 번호는 -Speeds의 순서다(0부터). summarize.py가 조건 "V<번호>-<구성>"으로 요약한다.
# 순서: 묶음 맨 앞에 r0 하나(버림), 그 뒤 회차가 바깥 루프다(홀수 회차는 조건 순서대로, 짝수 회차는 거꾸로).
# 실행 절차(앞 창 검사, 실패 CSV 이동, progress.log, failures.csv)는 run-pool.ps1에서 옮겼다. 메타데이터는 spike-check.py가 대조한다.
# 총괄 묶음의 조건 표(full-conditions.json)는 exe 경로와 공통 인자만 읽고 고치지 않는다.
param(
    [string]$Batch = "Spike1",
    [int]$N = 10000,
    # v2 W6: 쉼표로 이은 개체 수. 주면 -N 대신 N마다 조건이 갈린다(라벨의 N<N>, summarize.py는 N을 조건 열로 가른다). 예: "500,1000,2000"
    [string]$Ns = "",
    [Parameter(Mandatory = $true)][string]$RadiusM,
    [Parameter(Mandatory = $true)][string]$HeightM,
    # 쉼표로 이은 궤도 속도(m/s). 예: "0,1.5,3,6". -File로 부를 때 배열이 한 문자열로 넘어오므로 문자열로 받는다
    [Parameter(Mandatory = $true)][string]$Speeds,
    [int]$Runs = 5,
    # 쉼표로 이은 구성 이름. 예: "s8,a8sep"
    [string]$Configs = "s8",
    # v2 W10: 구성마다 개체 수를 따로 준다(쉼표로 이은 <구성>=<N>). 준 구성은 -N/-Ns 대신 이 N 하나로 잰다. 예: "s1=1000"(액터 기준선)
    [string]$ConfigNs = "",
    # v2 W10: 구성마다 회차 수를 따로 준다(쉼표로 이은 <구성>=<회차>). 준 구성은 -Runs 대신 이 회차까지만 잰다. 예: -Runs 1 -ConfigRuns "s8=3"
    [string]$ConfigRuns = "",
    [int]$DurationSec = 180,
    [int]$WarmupSec = -1,
    [int]$StartupSec = 30,
    [switch]$SkipR0,
    # v2 W7: 쉼표로 이은 조건 이름(V<속도 번호>-<구성>)은 재지 않는다. 예: "V0-finalcap500,V0-finalcap500hold0"(상한 조건은 이동에서만 잰다)
    [string]$SkipConds = "",
    # Insights 트레이스를 Saved/Traces/<라벨>.utrace로 남긴다. 트레이스 실행의 수치는 비교에 쓰지 않는다(이벤트 기록 비용)
    [switch]$Trace,
    [switch]$DryRun
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"

$WarmupEst = if ($WarmupSec -ge 0) { $WarmupSec } else { 60 }
$TimeoutSec = 3 * ($StartupSec + $WarmupEst + $DurationSec + 2)
$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Exe = "$ProjectDir\$(($Doc.exe) -replace '/', '\')"
$PkgRoot = "$ProjectDir\Packaged\Test\Windows\HordeOptLab"
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$BatchDir = "$ProjectDir\Measurements\$Batch"
$ProgressLog = "$BatchDir\progress.log"
$FailuresCsv = "$BatchDir\failures.csv"

# v2 W10: 구성은 사다리 단계(s1~s8, -LabStage=1~8)와 단계 8의 빼 보기(a8*)다. 옛 단계·변형 이름(M0~M5, -LabVariant)은 지웠고 옛 구성(final, m1 등)도 함께 지웠다.
# 단계별 스위치 표와 빼 보기 표는 Docs/Worklog/V2.md "W10"과 Docs/Guides/commands.md에 있다. 기대값은 그 표의 CSV 메타데이터 값이다.
$StepExpect = @{
    "1" = @("LabStage=1", "LabLadder=1", "LabTechHydrate=0", "LabTechAvoid=0", "LabTechSeparate=1", "LabActorCollision=pawn", "LabRepHold=0")
    "2" = @("LabStage=2", "LabTechInstAnim=0", "LabTechHydrate=1", "LabTechAnimBudget=0", "LabTechHydrateFix=0", "LabTechSimLod=0", "LabTechFarCull=1", "LabTechAvoid=1", "LabTechSeparate=0", "LabActorCollision=trace", "LabRepHold=0", "LabActorCap=500", "LabHydrateM=30")
    "3" = @("LabStage=3", "LabTechInstAnim=0", "LabTechHydrate=1", "LabTechAnimBudget=0", "LabTechHydrateFix=0", "LabTechSimLod=0", "LabTechAvoid=0", "LabTechSeparate=1", "LabActorCollision=trace", "LabRepHold=0")
    "4" = @("LabStage=4", "LabTechInstAnim=1", "LabTechAnimBudget=1", "LabTechHydrateFix=1", "LabTechPool=0", "LabTechSimLod=0", "LabTechIskm=0", "LabTechAvoid=0", "LabTechSeparate=1", "LabActorCollision=trace", "LabRepHold=2000")
    "5" = @("LabStage=5", "LabTechInstAnim=1", "LabTechPool=1", "LabTechSimLod=1", "LabTechIskm=0", "LabTechSeparate=1", "LabActorCollision=trace", "LabRepHold=2000")
    "6" = @("LabStage=6", "LabTechIskm=1", "LabTechFixedBounds=1", "LabTechCrowdFx=0", "LabBoundsStepM=10", "LabIskmTrackReserve=1", "LabTechSeparate=1", "LabRepHold=2000")
    "7" = @("LabStage=7", "LabTechIskm=1", "LabTechCrowdFx=1", "LabFxOrbFollow=1", "LabActorCap=500", "LabHydrateM=30", "LabFarShadow=1", "LabRepHold=2000")
    "8" = @("LabStage=8", "LabLadder=8", "LabTechSimLod=1", "LabTechIskm=1", "LabTechFixedBounds=1", "LabTechCrowdFx=1", "LabTechAvoid=0", "LabTechSeparate=1", "LabActorCollision=trace", "LabRepHold=2000",
            "LabFxOrbFollow=1", "LabBoundsStepM=10", "LabIskmTrackReserve=1", "LabActorCap=1000", "LabHydrateM=40", "LabFarShadow=0")
}
$ConfigTable = @()
foreach ($s in 1..8) { $ConfigTable += @{ Name = "s$s"; Args = @("-LabStage=$s"); Expect = @($StepExpect["$s"]) } }
# 빼 보기: 단계 8에서 스위치 하나. Expect는 바뀌는 메타데이터 값이다. LabOverrides는 정지 조건(orbit=…)과 섞이므로 대조하지 않는다
$Ablations = @(
    @{ Name = "a8hyd"; Arg = "-LabHydrate=0"; Expect = @("LabTechHydrate=0", "LabActorCollision=-") },
    @{ Name = "a8bud"; Arg = "-LabScreenSigBudget=0"; Expect = @("LabTechAnimBudget=0", "LabBudget=0") },
    @{ Name = "a8fix"; Arg = "-LabHydrateFixMask=0"; Expect = @("LabTechHydrateFix=0", "LabHydrateFixMask=0") },
    @{ Name = "a8hold"; Arg = "-LabRepHold=0"; Expect = @("LabRepHold=0") },
    @{ Name = "a8pool"; Arg = "-LabEntityPool=0"; Expect = @("LabTechPool=0", "LabPool=0") },
    @{ Name = "a8sim"; Arg = "-LabSimLod=0"; Expect = @("LabTechSimLod=0", "LabSimLod=-") },
    @{ Name = "a8iskm"; Arg = "-LabIskm=0"; Expect = @("LabTechIskm=0", "LabIskmTrackReserve=-", "LabBoundsStepM=-") },
    @{ Name = "a8bnd"; Arg = "-LabFixedBounds=0"; Expect = @("LabTechFixedBounds=0", "LabIskmBounds=0", "LabBoundsStepM=-") },
    @{ Name = "a8far"; Arg = "-LabFarCull=0"; Expect = @("LabTechFarCull=0") },
    @{ Name = "a8fx"; Arg = "-LabCrowdFx=0"; Expect = @("LabTechCrowdFx=0", "LabCrowdFxActive=0", "LabFxOrbFollow=-") },
    @{ Name = "a8sep"; Arg = "-LabSeparate=0"; Expect = @("LabTechSeparate=0") },
    @{ Name = "a8cap"; Arg = "-LabActorCollision=0"; Expect = @("LabActorCollision=0") },
    @{ Name = "a8res"; Arg = "-LabIskmTrackReserve=0"; Expect = @("LabIskmTrackReserve=0") },
    @{ Name = "a8fxf"; Arg = "-LabFxOrbFollow=0"; Expect = @("LabFxOrbFollow=0") },
    @{ Name = "a8stp"; Arg = "-LabBoundsStepM=0"; Expect = @("LabBoundsStepM=0") },
    @{ Name = "a8shd"; Arg = "-LabFarShadow=1"; Expect = @("LabFarShadow=1") },
    # 하위 실험(빼 보기가 아니라 더하기): 단계 8에 엔진 회피를 더한다(D43의 비교)
    @{ Name = "s8av"; Arg = "-LabAvoid=1"; Expect = @("LabTechAvoid=1") }
)
foreach ($a in $Ablations) {
    $keys = @($a.Expect | ForEach-Object { ($_ -split "=")[0] })
    $base = @($StepExpect["8"] | Where-Object { $keys -notcontains (($_ -split "=")[0]) })
    $ConfigTable += @{ Name = $a.Name; Args = @("-LabStage=8", $a.Arg); Expect = $base + @($a.Expect) }
}
# 직렬 트레이스용(-LabFullyParallel=0, Mass 프로세서가 Game 스레드에서 차례로 돈다). 수치는 비교에 쓰지 않는다
foreach ($s in 2..8) { $ConfigTable += @{ Name = "s${s}ser"; Args = @("-LabStage=$s", "-LabFullyParallel=0"); Expect = @($StepExpect["$s"]) } }
$ConfigNames = @($Configs -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$UseConfigs = @($ConfigNames | ForEach-Object { $cfgName = $_; $ConfigTable | Where-Object { $_.Name -eq $cfgName } }) # -Configs의 순서대로(r0은 첫 구성)
if ($UseConfigs.Count -ne $ConfigNames.Count) { Write-Host "FAIL: unknown config in -Configs '$Configs' ($(($ConfigTable | ForEach-Object { $_.Name }) -join ', '))"; exit 1 }
$SpeedList = @($Speeds -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$NList = if ($Ns) { @($Ns -split "," | ForEach-Object { [int]$_.Trim() } | Where-Object { $_ -gt 0 }) } else { @($N) }
$ConfigN = @{}
foreach ($pair in @($ConfigNs -split "," | Where-Object { $_.Trim() -ne "" })) { $k, $v = $pair.Trim() -split "="; $ConfigN[$k] = [int]$v }
$Conds = @()
for ($i = 0; $i -lt $SpeedList.Count; $i++) {
    foreach ($c in $UseConfigs) {
        $cn = if ($ConfigN.ContainsKey($c.Name)) { @($ConfigN[$c.Name]) } else { $NList }
        foreach ($n in $cn) { $Conds += @{ Step = "V$i"; Speed = $SpeedList[$i]; Config = $c; N = $n } }
    }
}
$SkipList = @($SkipConds -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
if ($SkipList.Count -gt 0) { $Conds = @($Conds | Where-Object { $SkipList -notcontains "$($_.Step)-$($_.Config.Name)" }) }
function Label-Of($c, $run) { return "$Batch-$($c.Step)-$($c.Config.Name)-N$($c.N)-P1-r$run" }
function Get-GameArgs($c, $label) {
    $a = @($Doc.common_args) + @($c.Config.Args) + @("-LabN=$($c.N)", "-LabPreset=1", "-LabLabel=$label",
        "-LabOrbOrbitRadius=$RadiusM", "-LabOrbOrbitSpeed=$($c.Speed)", "-LabOrbHeight=$HeightM", "-LabDuration=$DurationSec")
    if ($WarmupSec -ge 0) { $a += "-LabWarmup=$WarmupSec" }
    if ($Trace) { $a += @("-trace=cpu,gpu,frame,counters,mass", "-tracefile=`"$ProjectDir\Saved\Traces\$label.utrace`"") }
    return $a
}
function Get-Expect($c, $label) {
    return @("LabOrbit=r$RadiusM/v$($c.Speed)/h$HeightM", "LabPreset=P1", "LabN=$($c.N)", "LabLabel=$label", "LabAlive=$($c.N)", "LabOk=1",
        "LabTypes=2", "LabTypesAssets=ok", "LabHud=0", "LabShotAt=0") + @($c.Config.Expect)
}

# 실행 목록: r0(첫 조건, 버림) + 회차마다 전 조건
$Jobs = @()
if (-not $SkipR0) { $Jobs += @{ Cond = $Conds[0]; Run = 0 } }
$ConfigRun = @{}
foreach ($pair in @($ConfigRuns -split "," | Where-Object { $_.Trim() -ne "" })) { $k, $v = $pair.Trim() -split "="; $ConfigRun[$k] = [int]$v }
$MaxRuns = @(@($Runs) + @($ConfigRun.Values) | Measure-Object -Maximum).Maximum
for ($r = 1; $r -le $MaxRuns; $r++) {
    $order = if ($r % 2 -eq 1) { $Conds } else { $o = @($Conds); [array]::Reverse($o); $o }
    foreach ($c in $order) {
        $limit = if ($ConfigRun.ContainsKey($c.Config.Name)) { $ConfigRun[$c.Config.Name] } else { $Runs }
        if ($r -le $limit) { $Jobs += @{ Cond = $c; Run = $r } }
    }
}

if ($DryRun) {
    $est = [math]::Round($Jobs.Count * ($StartupSec + $WarmupEst + $DurationSec + 2) / 60.0, 1)
    Write-Host "DRYRUN batch=$Batch N=$($NList -join '/') radius=${RadiusM}m height=${HeightM}m speeds=$($SpeedList -join '/') conds=$($Conds.Count) runs=$($Jobs.Count) est=${est} min"
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
if ($Trace) { New-Item -ItemType Directory -Force -Path "$ProjectDir\Saved\Traces" | Out-Null }
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
    if ($reason) { Add-Content -Path $FailuresCsv -Encoding UTF8 -Value ("{0},{1},{2},{3},P1,spike,{4}" -f (Now), $Label, "$($c.Step)-$($c.Config.Name)", $c.N, (Clean $reason)) }
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
# 실패한 실행은 묶음 끝에서 새 회차 번호(회차 + 가장 큰 회차 수)로 한 번만 다시 잰다
$GaveUp = 0
foreach ($j in $Retry) {
    if (Invoke-Run $j.Cond ($j.Run + $MaxRuns)) { $Ok++ } else { $GaveUp++; Write-Host "GIVEUP $(Label-Of $j.Cond ($j.Run + $MaxRuns))" }
}
$Info = [ordered]@{ batch = $Batch; build = $Commit; start = $Start; end = (Now); n = ($NList -join ","); preset = "P3"; radius_m = $RadiusM; height_m = $HeightM; speeds = $SpeedList;
    duration_sec = $DurationSec; runs = $Runs; planned = $Jobs.Count; ok = $Ok; failed = $Fail; gave_up = $GaveUp }
$Info | ConvertTo-Json | Set-Content -Path "$BatchDir\batch.json" -Encoding UTF8
Write-Host "DONE $Batch ended=$(Now) ok=$Ok failed=$Fail gave_up=$GaveUp"
exit 0
