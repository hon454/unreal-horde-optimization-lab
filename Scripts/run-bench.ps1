# Test 패키지를 -LabBench로 띄워 조건 하나를 반복해 잰다. 묶음의 첫 실행 r0은 셰이더 캐시용이고 summary에서 빠진다.
# 라벨: <Milestone>-<Variant>-N<N>-<Preset>-r<i>. CSV는 패키지의 Saved/Profiling/CSV에서 프로젝트 Saved/Profiling/CSV로 가져온다.
# 같은 라벨의 CSV가 이미 있으면 거부한다(AGENTS.md "같은 라벨은 다시 쓰지 않는다"). 실패한 실행은 -Tag로 라벨을 바꿔 다시 잰다.
param(
    [Parameter(Mandatory)][string]$Milestone,
    [Parameter(Mandatory)][int]$N,
    [ValidateSet("P1", "P2", "P3")][string]$Preset = "P1",
    [ValidateSet("full", "static", "budget", "uro")][string]$Variant = "full",
    [int]$Seed = 1,
    [int]$Runs = 3,
    [string]$Tag = "",
    [int]$ResX = 1920,
    [int]$ResY = 1080
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
for ($i = 0; $i -le $Runs; $i++) {
    $Label = "$Base-r$i"
    if (Test-Path "$OutCsv\$Label.csv") { Write-Host "FAIL: $Label.csv already exists. Use -Tag."; exit 1 }
    $GameArgs = @("-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosound", "-LOG=$Label.log",
        "-LabBench", "-LabN=$N", "-LabPreset=$($Preset.Substring(1))", "-LabSeed=$Seed", "-LabVariant=$Variant", "-LabLabel=$Label")
    Write-Host "RUN $Label"
    $Proc = Start-Process -FilePath $Exe -ArgumentList $GameArgs -PassThru
    $Proc.WaitForExit()
    $Log = "$ProjectDir\Packaged\Test\Windows\HordeOptLab\Saved\Logs\$Label.log"
    if (Test-Path $Log) { Select-String -Path $Log -Pattern "lab_summary" | ForEach-Object { $_.Line } }
    if ($Proc.ExitCode -ne 0 -or -not (Test-Path "$PkgCsv\$Label.csv")) {
        Write-Host "FAIL: $Label exit=$($Proc.ExitCode) csv=$(Test-Path "$PkgCsv\$Label.csv")"
        $Failed++
        continue
    }
    Move-Item "$PkgCsv\$Label.csv" "$OutCsv\$Label.csv"
}
Write-Host "DONE: $Base runs=$($Runs + 1) failed=$Failed"
exit $(if ($Failed -gt 0) { 1 } else { 0 })
