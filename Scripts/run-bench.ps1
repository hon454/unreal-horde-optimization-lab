# Test 패키지를 -LabBench로 띄워 조건 하나를 반복해 잰다. -Milestone이 단계(-LabStage)다(ADR-0004).
# 묶음의 맨 첫 실행 r0만 드라이버 PSO 캐시용으로 버린다(summary에서 빠진다). 묶음의 나머지 조건은 -SkipR0으로 r1부터 잰다.
# 라벨: <Milestone>-<Variant>-N<N>-<Preset>-r<i>. CSV는 패키지의 Saved/Profiling/CSV에서 프로젝트 Saved/Profiling/CSV로 가져온다.
# 같은 라벨의 CSV가 이미 있으면 거부한다(AGENTS.md "같은 라벨은 다시 쓰지 않는다"). 실패한 실행은 -Tag로 라벨을 바꿔 다시 잰다.
param(
    [Parameter(Mandatory)][string]$Milestone,
    [Parameter(Mandatory)][int]$N,
    [ValidateSet("P1", "P2")][string]$Preset = "P1",
    [ValidateSet("full", "static", "budget", "uro", "pawn", "mass", "massst", "massav", "massdirlo", "massdirhi", "massevtlo", "massevthi", "vatvert", "vatcull", "vatcullvis", "vatdie0", "vatdie1k", "kill0", "kill1k", "m3uro", "m3budgetdist", "m3budgetsig", "m3lowres", "m3colquery", "m3nocol", "m3colbase", "m3hyd15", "m3hyd50", "m3hydbase", "m2off", "m3noactor", "m3base", "noanim", "m3vis0", "m3vis15", "m3hys20", "m3qold", "m3syncall", "m3lock0", "m3order", "m3vis30", "m3sig", "m0full", "m0static", "staticpawn", "m1mass", "m2mass", "m3mass", "m4mass", "m4off", "m4m25l50", "m4m25l100", "m4m50l50", "m4avoid", "m4offmove", "m4offpool", "m5iskm", "m5destroy", "m5blend", "m5minss")][string]$Variant = "full",
    [int]$Seed = 20261106,
    [int]$Runs = 3,
    [string]$Tag = "",
    [int]$ResX = 2560,
    [int]$ResY = 1440,
    [switch]$SkipR0,
    # 결과는 -Milestone 폴더·라벨로 두고 실행 단계만 바꾼다. M2 E2 (a)가 M1을 같은 묶음에서 잴 때 쓴다(변형 noanim, 2026-11-08).
    [string]$Stage = "",
    # M3b: 실행 하나만 잰다(r<RunIndex>). 묶음 러너가 실행 단위로 섞을 때 쓴다(A r1, B r1, A r2, ...). -1이면 r1..Runs(-SkipR0가 아니면 r0부터).
    [int]$RunIndex = -1,
    # M3b: Insights 트레이스를 Saved/Traces/<라벨>.utrace로 남긴다(-trace=cpu,gpu,frame,counters,mass). 트레이스 실행의 수치는 비교에 쓰지 않는다.
    [switch]$Trace,
    # 게임 인자를 그대로 덧붙인다(예: 직렬 트레이스의 "-LabFullyParallel=0", 인자 이름은 코드 세션이 정한다).
    [string]$ExtraArgs = "",
    # 프로세스 시작 직후 CPU 어피니티 마스크(16진수, 예: FFFF)를 건다. 없으면 걸지 않는다. 잡음 진단용(5950X 두 CCD). 로그와 Saved/Lab/<라벨>.summary.txt에 affinity=0x...를 남긴다.
    [string]$Affinity = "",
    # 개체 종류 수. 1이면 -LabTypes=1, 2면 -LabTypes=2를 항상 붙인다(코드 기본값이 M4부터 2라서 1도 명시해야 한다, Worklog/M4.md). 라벨은 -Tag로 따로 구분한다(예: t2). 기본값 1은 M3b까지의 호출과 같은 뜻이다.
    [ValidateSet(1, 2)][int]$Types = 1,
    # 그리는 거리(m). 0이면 -LabFarM을 붙이지 않는다(코드 기본 200). 200보다 작으면 코드가 200으로 올린다. 라벨은 -Tag로 구분한다(예: far300). Worklog/M4.md "그리는 거리 인자".
    [int]$FarM = 0,
    # 포그라운드 검사를 끈다. 기본은 켜짐: 게임 창을 앞에 두고(Start-Process 뒤), 측정 구간 동안 2초마다 포그라운드 창을 기록해
    # 게임이 아닌 표본이 하나라도 있으면 그 실행을 FAIL(CSV를 Saved/Profiling/CSV/fg-failed/로 옮김)로 한다. 이유: 창이 백그라운드이면 Game 스레드가 약 2ms(12%) 느리다(Docs/Worklog/M3b.md "페이즈 첫 실행 효과").
    [switch]$NoFgCheck
)
. "$PSScriptRoot\common.ps1"

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
# 게임 창을 앞에 둔다. 1) MainWindowHandle을 최대 30초(0.5초 간격) 기다린다 2) SetForegroundWindow 3) WScript.Shell AppActivate(PID) 4) Alt 키 눌림 뒤 SetForegroundWindow. 결과 문자열을 돌려준다.
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

$Exe = "$ProjectDir\Packaged\Test\Windows\HordeOptLab\Binaries\Win64\HordeOptLab-Win64-Test.exe"
if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe. Run Scripts/package.ps1 first."; exit 1 }
$Running = @(Get-Process UnrealEditor, HordeOptLab-Win64-Test -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) { Write-Host "FAIL: another run or editor is up (pid $($Running.Id -join ', '))."; exit 1 }

$PkgCsv = "$ProjectDir\Packaged\Test\Windows\HordeOptLab\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
New-Item -ItemType Directory -Force -Path $OutCsv | Out-Null
$Base = "$Milestone-$Variant-N$N-$Preset$Tag"

$Failed = 0
$First = if ($SkipR0) { 1 } else { 0 }
$Last = $Runs
if ($RunIndex -ge 0) { $First = $RunIndex; $Last = $RunIndex }
if ($Trace) { New-Item -ItemType Directory -Force -Path "$ProjectDir\Saved\Traces" | Out-Null }
for ($i = $First; $i -le $Last; $i++) {
    $Label = "$Base-r$i"
    if (Test-Path "$OutCsv\$Label.csv") { Write-Host "FAIL: $Label.csv already exists. Use -Tag."; exit 1 }
    $GameArgs = @("-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosound",
        "-LabBench", "-LabStage=$(if ($Stage) { $Stage } else { $Milestone })", "-LabN=$N", "-LabPreset=$($Preset.Substring(1))", "-LabSeed=$Seed", "-LabVariant=$Variant", "-LabLabel=$Label")
    if ($Trace) { $GameArgs += @("-trace=cpu,gpu,frame,counters,mass", "-tracefile=`"$ProjectDir\Saved\Traces\$Label.utrace`"") }
    $GameArgs += "-LabTypes=$Types"
    if ($FarM -gt 0) { $GameArgs += "-LabFarM=$FarM" }
    if ($ExtraArgs) { $GameArgs += $ExtraArgs.Split(" ", [StringSplitOptions]::RemoveEmptyEntries) }
    Write-Host "RUN $Label"
    $Proc = Start-Process -FilePath $Exe -ArgumentList $GameArgs -PassThru
    $AffNote = "affinity=none"
    if ($Affinity) {
        $Hex = $Affinity -replace "^0x", ""
        $Mask = [Convert]::ToInt64($Hex, 16)
        try { $Proc.ProcessorAffinity = [IntPtr]$Mask; $AffNote = "affinity=0x{0:X}" -f $Mask } catch { Write-Host "WARN: could not set affinity: $_"; $AffNote = "affinity=FAILED" }
    }
    Write-Host $AffNote
    $FocusNote = "focus=skipped"
    $Samples = New-Object System.Collections.ArrayList
    if (-not $NoFgCheck) {
        $FocusNote = Set-GameForeground $Proc
        Write-Host $FocusNote
        # 2초마다 포그라운드 창의 PID를 기록한다(프로세스가 끝날 때까지). 측정 구간 표본만 아래에서 센다.
        while (-not $Proc.HasExited) {
            [void]$Samples.Add(@{ T = (Get-Date); Pid = (Get-FgPid) })
            Start-Sleep -Milliseconds 2000
        }
    }
    $Proc.WaitForExit()
    # Test 구성은 로그 파일을 남기지 않는다(engine-notes.md 6절 NO_LOGGING). 게임이 Saved/Lab/<라벨>.summary.txt에 요약을 쓴다.
    $Summary = "$ProjectDir\Packaged\Test\Windows\HordeOptLab\Saved\Lab\$Label.summary.txt"
    if ($Affinity -and (Test-Path $Summary)) { Add-Content -Path $Summary -Value $AffNote }
    # 측정 구간 = summary.txt 마지막 기록 시각 - measure_sec. 그 안의 표본 중 게임 PID가 아닌 것을 센다.
    $FgFail = $false
    if (-not $NoFgCheck -and (Test-Path $Summary)) {
        $Text = Get-Content $Summary -Raw
        $Sec = [double]([regex]::Match($Text, "measure_sec=([0-9.]+)").Groups[1].Value)
        $End = (Get-Item $Summary).LastWriteTime
        $Begin = $End.AddSeconds(-$Sec)
        $In = @($Samples | Where-Object { $_.T -ge $Begin -and $_.T -le $End })
        $Non = @($In | Where-Object { $_.Pid -ne $Proc.Id }).Count
        $FgNote = "fg=$($In.Count - $Non)/$($In.Count) $FocusNote"
        Add-Content -Path $Summary -Value ("`n" + $FgNote)
        Write-Host $FgNote
        if ($In.Count -eq 0 -or $Non -gt 0) { $FgFail = $true }
    }
    if (Test-Path $Summary) { Get-Content $Summary }
    if ($FgFail -and (Test-Path "$PkgCsv\$Label.csv")) {
        $FailDir = "$OutCsv\fg-failed"; New-Item -ItemType Directory -Force -Path $FailDir | Out-Null
        $Stamp = Get-Date -Format "yyyyMMddHHmmss"
        Move-Item "$PkgCsv\$Label.csv" "$FailDir\$Label-$Stamp.csv"
        Write-Host "FAIL: $Label foreground not on game during measurement (non-game samples=$Non of $($In.Count)). CSV moved to fg-failed/."
        $Failed++
        continue
    }
    if ($Proc.ExitCode -ne 0 -or -not (Test-Path "$PkgCsv\$Label.csv")) {
        Write-Host "FAIL: $Label exit=$($Proc.ExitCode) csv=$(Test-Path "$PkgCsv\$Label.csv")"
        $Failed++
        continue
    }
    Move-Item "$PkgCsv\$Label.csv" "$OutCsv\$Label.csv"
}
Write-Host "DONE: $Base runs=$($Last + 1 - $First) failed=$Failed"
exit $(if ($Failed -gt 0) { 1 } else { 0 })
