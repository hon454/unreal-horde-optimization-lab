# Development 에디터 빌드를 -game으로 실행한다. 개발·동작 확인·오버레이 스크린샷용이고 수치는 비교에 쓰지 않는다(ADR-0001).
#   -Small: N=100, P1, 20초 뒤 자동 종료. 완료 확인(AGENTS.md "완료 확인" 2번)에 쓴다.
param(
    [int]$N = 500,
    [ValidateSet("P1", "P2")][string]$Preset = "P1",
    [int]$Seed = 20261106,
    [ValidateSet("full", "static", "budget", "uro", "pawn", "mass", "massst", "massav", "massdirlo", "massdirhi", "massevtlo", "massevthi", "vatvert", "vatcull", "vatcullvis", "vatdie0", "vatdie1k", "kill0", "kill1k", "m3uro", "m3budgetdist", "m3budgetsig", "m3lowres", "m3colquery", "m3nocol", "m3colbase", "m3hyd15", "m3hyd50", "m3hydbase", "m2off", "m3noactor", "m3base", "m3qold", "m3syncall", "m3lock0", "m3order", "m3vis30", "m3vis0", "m3vis15", "m3hys20", "m0full", "m0static", "staticpawn", "m1mass", "m2mass", "m3mass")][string]$Variant = "full",
    [double]$Duration = 0,
    [double]$ShotAt = 0,
    [string[]]$Extra = @(),
    [switch]$Small
)
. "$PSScriptRoot\common.ps1"

if ($Small) { $N = 100; $Preset = "P1"; if ($Duration -le 0) { $Duration = 20 } }

$LogName = "LabDev-$Variant-N$N-$Preset.log"
$GameArgs = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=1280", "-ResY=720", "-log", "-LOG=$LogName", "-nosound", "-DisablePython",
    "-LabN=$N", "-LabPreset=$($Preset.Substring(1))", "-LabSeed=$Seed", "-LabVariant=$Variant"
) + $Extra
if ($Duration -gt 0) { $GameArgs += "-LabDuration=$Duration" }
if ($ShotAt -gt 0) { $GameArgs += "-LabShotAt=$ShotAt" }

Write-Host "RUN: $Editor $($GameArgs -join ' ')"
# -Wait는 자식 프로세스(crashpad_handler 등)까지 기다려 끝나지 않는다. 본 프로세스만 기다린다.
$Proc = Start-Process -FilePath $Editor -ArgumentList $GameArgs -PassThru
$Proc.WaitForExit()
$Log = "$ProjectDir\Saved\Logs\$LogName"
if (Test-Path $Log) {
    Select-String -Path $Log -Pattern "lab_config|lab_assets|lab_spawned|lab_summary" | ForEach-Object { $_.Line }
}
Write-Host "EXIT: $($Proc.ExitCode)"
exit $Proc.ExitCode
