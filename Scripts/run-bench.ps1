# Test 패키지를 -LabBench로 띄워 조건 하나를 반복해 잰다. -Milestone이 단계(-LabStage)다(ADR-0004).
# 묶음의 맨 첫 실행 r0만 드라이버 PSO 캐시용으로 버린다(summary에서 빠진다). 묶음의 나머지 조건은 -SkipR0으로 r1부터 잰다.
# 라벨: <Milestone>-<Variant>-N<N>-<Preset>-r<i>. CSV는 패키지의 Saved/Profiling/CSV에서 프로젝트 Saved/Profiling/CSV로 가져온다.
# 같은 라벨의 CSV가 이미 있으면 거부한다(AGENTS.md "같은 라벨은 다시 쓰지 않는다"). 실패한 실행은 -Tag로 라벨을 바꿔 다시 잰다.
param(
    [Parameter(Mandatory)][string]$Milestone,
    [Parameter(Mandatory)][int]$N,
    [ValidateSet("P1", "P2")][string]$Preset = "P1",
    [ValidateSet("full", "static", "budget", "uro", "pawn", "mass", "massst", "massav", "massdirlo", "massdirhi", "massevtlo", "massevthi", "vatvert", "vatcull", "vatcullvis", "vatdie0", "vatdie1k")][string]$Variant = "full",
    [int]$Seed = 20261106,
    [int]$Runs = 3,
    [string]$Tag = "",
    [int]$ResX = 2560,
    [int]$ResY = 1440,
    [switch]$SkipR0
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
for ($i = $First; $i -le $Runs; $i++) {
    $Label = "$Base-r$i"
    if (Test-Path "$OutCsv\$Label.csv") { Write-Host "FAIL: $Label.csv already exists. Use -Tag."; exit 1 }
    $GameArgs = @("-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosound",
        "-LabBench", "-LabStage=$Milestone", "-LabN=$N", "-LabPreset=$($Preset.Substring(1))", "-LabSeed=$Seed", "-LabVariant=$Variant", "-LabLabel=$Label")
    Write-Host "RUN $Label"
    $Proc = Start-Process -FilePath $Exe -ArgumentList $GameArgs -PassThru
    $Proc.WaitForExit()
    # Test 구성은 로그 파일을 남기지 않는다(engine-notes.md 6절 NO_LOGGING). 게임이 Saved/Lab/<라벨>.summary.txt에 요약을 쓴다.
    $Summary = "$ProjectDir\Packaged\Test\Windows\HordeOptLab\Saved\Lab\$Label.summary.txt"
    if (Test-Path $Summary) { Get-Content $Summary }
    if ($Proc.ExitCode -ne 0 -or -not (Test-Path "$PkgCsv\$Label.csv")) {
        Write-Host "FAIL: $Label exit=$($Proc.ExitCode) csv=$(Test-Path "$PkgCsv\$Label.csv")"
        $Failed++
        continue
    }
    Move-Item "$PkgCsv\$Label.csv" "$OutCsv\$Label.csv"
}
Write-Host "DONE: $Base runs=$($Runs + 1 - $First) failed=$Failed"
exit $(if ($Failed -gt 0) { 1 } else { 0 })
