# v2 W9 ISKM 번쩍임 진단(Docs/Worklog/V2.md "W9"): 개발 빌드 최종 구성(1만, 오브 궤도 기본값 2m/s, P1)을 고정 1/60초로 돌려 ShotAt부터 Burst 프레임을 연속 캡처한다.
# -LabFlashDiag로 프레임마다 ISKM 컴포넌트에 들어온 개체(lab_flash)와 그 화면 좌표(lab_event iskm_enter, iskm_state)를 로그에 남긴다.
# 캡처 폴더를 Saved/Screenshots/LabBurst-w9-<Name>으로, 로그를 그 안으로 옮긴다. 후보는 -Cmds(콘솔 명령, 쉼표로 구분)나 -Args2(게임 인자)로 하나씩 바꾼다.
# 예: powershell -ExecutionPolicy Bypass -File Scripts/run-flash.ps1 -Name reserve0 -Args2 "-LabIskmTrackReserve=0"
#     속도 버퍼: -Cmds "r.BufferVisualizationTarget Velocity,ShowFlag.VisualizeBuffer 1", 확대: -Args2 "-LabDiagFov=30","-LabDiagAimM=90","-LabDiagYaw=0"
#     이어서 py -3 -I Scripts/flash-scan.py Saved/Screenshots/LabBurst-w9-reserve0 로 한 프레임 튐을 센다.
param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Cmds = "",
    [string]$Variant = "q4final",
    [string]$Stage = "M5",
    [int]$N = 10000,
    [double]$ShotAt = 60,
    [int]$Burst = 1800,
    [double]$Duration = 92,
    [string[]]$Args2 = @()
)
. "$PSScriptRoot\common.ps1"
$Shots = "$ProjectDir\Saved\Screenshots"
$Out = "$Shots\LabBurst-w9-$Name"
if (Test-Path $Out) { throw "$Out exists. Use a new -Name." }
if ((Test-Path "$Shots\LabBurst") -and (Get-ChildItem "$Shots\LabBurst" -File | Select-Object -First 1)) { throw "Saved/Screenshots/LabBurst is not empty. Move or delete the previous capture first." }
$Extra = @("-benchmark", "-fps=60", "-LabShotBurst=$Burst", "-LabFlashDiag")
if ($Cmds) { $Extra += "-ExecCmds=`"$Cmds`"" }
$Extra += $Args2
# 같은 프로세스에서 부른다(powershell -File로 다시 띄우면 공백 든 인자가 쪼개진다, Worklog/M5.md "미니언 깜빡임 진단")
& "$PSScriptRoot\run-dev.ps1" -N $N -Preset P1 -Duration $Duration -ShotAt $ShotAt -Variant $Variant -Stage $Stage -Extra $Extra
$Code = $LASTEXITCODE
if (Test-Path "$Shots\LabBurst") { Move-Item "$Shots\LabBurst" $Out } else { New-Item -ItemType Directory -Force -Path $Out | Out-Null }
Copy-Item "$ProjectDir\Saved\Logs\LabDev-$Variant-N$N-P1.log" "$Out\run.log"
Write-Host "BURST: $Out EXIT: $Code"
exit $Code
