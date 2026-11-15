# Development 에디터 빌드를 -game으로 실행한다. 개발·동작 확인·오버레이 스크린샷용이고 수치는 비교에 쓰지 않는다(ADR-0001).
#   -Small: N=100, P1, 20초 뒤 자동 종료. 완료 확인(AGENTS.md "완료 확인" 2번)에 쓴다.
#   -Stage: 사다리 단계 1~8(-LabStage=, Docs/v2-plan.md "사다리"). 기본 8(최종 구성). 빼 보기는 -Extra로 스위치를 준다(Docs/Guides/commands.md).
param(
    [int]$N = 500,
    [ValidateSet("P1", "P2")][string]$Preset = "P1",
    [int]$Seed = 20261106,
    [ValidateSet("1", "2", "3", "4", "5", "6", "7", "8")][string]$Stage = "8",
    # 로그 파일 이름에 붙는 꼬리표(같은 단계를 인자만 바꿔 여러 번 돌릴 때). 비우면 붙이지 않는다
    [string]$Tag = "",
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [double]$Duration = 0,
    [double]$ShotAt = 0,
    [string[]]$Extra = @(),
    [switch]$Small
)
. "$PSScriptRoot\common.ps1"

if ($Small) { $N = 100; $Preset = "P1"; if ($Duration -le 0) { $Duration = 20 } }

$LogName = "LabDev-s$Stage$(if ($Tag) { "-$Tag" })-N$N-$Preset.log"
$GameArgs = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-log", "-LOG=$LogName", "-nosound", "-DisablePython",
    "-LabN=$N", "-LabPreset=$($Preset.Substring(1))", "-LabSeed=$Seed", "-LabStage=$Stage"
) + $Extra
if ($Duration -gt 0) { $GameArgs += "-LabDuration=$Duration" }
if ($ShotAt -gt 0) { $GameArgs += "-LabShotAt=$ShotAt" }

Write-Host "RUN: $Editor $($GameArgs -join ' ')"
# -Wait는 자식 프로세스(crashpad_handler 등)까지 기다려 끝나지 않는다. 본 프로세스만 기다린다.
$Proc = Start-Process -FilePath $Editor -ArgumentList $GameArgs -PassThru
$Proc.WaitForExit()
$Log = "$ProjectDir\Saved\Logs\$LogName"
if (Test-Path $Log) {
    Select-String -Path $Log -Pattern "lab_config|lab_ism_processor|lab_assets|lab_spawned|lab_applied|lab_summary" | ForEach-Object { $_.Line }
}
Write-Host "EXIT: $($Proc.ExitCode)"
exit $Proc.ExitCode
