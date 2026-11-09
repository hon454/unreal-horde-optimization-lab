# Test 패키지를 -LabBench로 띄워 조건 하나를 반복해 잰다. -Milestone이 단계(-LabStage)다(ADR-0004).
# 묶음의 맨 첫 실행 r0만 드라이버 PSO 캐시용으로 버린다(summary에서 빠진다). 묶음의 나머지 조건은 -SkipR0으로 r1부터 잰다.
# 라벨: <Milestone>-<Variant>-N<N>-<Preset>-r<i>. CSV는 패키지의 Saved/Profiling/CSV에서 프로젝트 Saved/Profiling/CSV로 가져온다.
# 같은 라벨의 CSV가 이미 있으면 거부한다(AGENTS.md "같은 라벨은 다시 쓰지 않는다"). 실패한 실행은 -Tag로 라벨을 바꿔 다시 잰다.
param(
    [Parameter(Mandatory)][string]$Milestone,
    [Parameter(Mandatory)][int]$N,
    [ValidateSet("P1", "P2")][string]$Preset = "P1",
    [ValidateSet("full", "static", "budget", "uro", "pawn", "mass", "massst", "massav", "massdirlo", "massdirhi", "massevtlo", "massevthi", "vatvert", "vatcull", "vatcullvis", "vatdie0", "vatdie1k", "kill0", "kill1k", "m3uro", "m3budgetdist", "m3budgetsig", "m3lowres", "m3colquery", "m3nocol", "m3colbase", "m3hyd15", "m3hyd50", "m3hydbase", "m2off", "m3noactor", "m3base", "noanim", "m3vis0", "m3vis15", "m3hys20", "m3qold", "m3syncall", "m3lock0", "m3noorder", "m0full", "m0static", "staticpawn", "m1mass", "m2mass", "m3mass")][string]$Variant = "full",
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
    [string]$Affinity = ""
)
. "$PSScriptRoot\common.ps1"

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
    $Proc.WaitForExit()
    # Test 구성은 로그 파일을 남기지 않는다(engine-notes.md 6절 NO_LOGGING). 게임이 Saved/Lab/<라벨>.summary.txt에 요약을 쓴다.
    $Summary = "$ProjectDir\Packaged\Test\Windows\HordeOptLab\Saved\Lab\$Label.summary.txt"
    if ($Affinity -and (Test-Path $Summary)) { Add-Content -Path $Summary -Value $AffNote }
    if (Test-Path $Summary) { Get-Content $Summary }
    if ($Proc.ExitCode -ne 0 -or -not (Test-Path "$PkgCsv\$Label.csv")) {
        Write-Host "FAIL: $Label exit=$($Proc.ExitCode) csv=$(Test-Path "$PkgCsv\$Label.csv")"
        $Failed++
        continue
    }
    Move-Item "$PkgCsv\$Label.csv" "$OutCsv\$Label.csv"
}
Write-Host "DONE: $Base runs=$($Last + 1 - $First) failed=$Failed"
exit $(if ($Failed -gt 0) { 1 } else { 0 })
