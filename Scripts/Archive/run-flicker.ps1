# 미니언 깜빡임 진단(Worklog/M5.md "미니언 깜빡임 진단"): 개발 빌드 20k P1을 고정 1/60초로 돌려 ShotAt부터 Burst 프레임을 연속 캡처하고,
# 캡처 폴더를 Saved/Screenshots/LabBurst_<Name>으로, 로그를 그 안으로 옮긴다. 후보 하나를 -Cmds(콘솔 명령, 쉼표로 구분)로 끈다.
# 예: powershell -ExecutionPolicy Bypass -File Scripts/run-flicker.ps1 -Name nomb -Cmds "r.MotionBlurQuality 0"
#     이어서 py -3 Scripts/flicker-scan.py Saved/Screenshots/LabBurst_nomb <csv> 로 한 프레임 밝기 튐을 센다.
param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Cmds = "",
    [string]$Variant = "m5bounds",
    [string]$Stage = "M5",
    [double]$ShotAt = 55,
    [int]$Burst = 720,
    [double]$Duration = 68,
    [string[]]$Args2 = @()  # 게임 인자 추가(예: -LabScreenSigBudget=0)
)
. "$PSScriptRoot\common.ps1"
$Shots = "$ProjectDir\Saved\Screenshots"
if (Test-Path "$Shots\LabBurst") { throw "Saved/Screenshots/LabBurst가 남아 있다. 앞 캡처를 옮기거나 지운 뒤 돌린다." }
$Extra = @("-benchmark", "-fps=60", "-LabShotBurst=$Burst")
if ($Cmds) { $Extra += "-ExecCmds=`"$Cmds`"" }
$Extra += $Args2
# 같은 프로세스에서 부른다(powershell -File로 다시 띄우면 공백 든 인자가 쪼개져 위치 인자 -Seed에 들어간다)
& "$PSScriptRoot\run-dev.ps1" -N 20000 -Preset P1 -Duration $Duration -ShotAt $ShotAt -Variant $Variant -Stage $Stage -Extra $Extra
$Code = $LASTEXITCODE
Move-Item "$Shots\LabBurst" "$Shots\LabBurst_$Name"
Copy-Item "$ProjectDir\Saved\Logs\LabDev-$Variant-N20000-P1.log" "$Shots\LabBurst_$Name\run.log"
Write-Host "BURST: Saved/Screenshots/LabBurst_$Name EXIT: $Code"
exit $Code
