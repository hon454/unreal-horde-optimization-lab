# Insights 트레이스(.utrace)에서 표를 뽑는다(UnrealInsights.exe -NoUI -AutoQuit, 명령줄 내보내기). M4 트레이스 분석(Docs/Worklog/M4.md "트레이스 분석").
# UnrealInsights.exe는 소스 빌드 엔진에 없어 한 번 만든다: Engine\Build\BatchFiles\Build.bat UnrealInsights Win64 Development
# 내보내기 파일 경로에 공백이 있으면 명령이 거기서 잘리므로 공백 없는 -OutDir을 쓴다(기본 %TEMP%\horde-trace).
# 예: Scripts/trace-export.ps1 -Label M4-m4mass-N20000-P1traceser-r1 -Command Threads
#     Scripts/trace-export.ps1 -Label ... -Command TimerStatistics -Extra "-threads=GameThread -startTime=20 -endTime=80 -columns=* -sortBy=TotalExclusiveTime"
param(
    [Parameter(Mandatory)][string]$Label,
    [Parameter(Mandatory)][ValidateSet("Threads", "Timers", "TimerStatistics", "TimingEvents")][string]$Command,
    [string]$Extra = "",
    [string]$Name = "",
    [string]$OutDir = "$env:TEMP\horde-trace"
)
$Root = Split-Path $PSScriptRoot -Parent
. "$PSScriptRoot\common.ps1"
$Insights = "$UE_ROOT\Engine\Binaries\Win64\UnrealInsights.exe"
$Trace = "$Root\Saved\Traces\$Label.utrace"
if (-not (Test-Path $Insights)) { Write-Host "FAIL: no UnrealInsights.exe (see header)"; exit 1 }
if (-not (Test-Path $Trace)) { Write-Host "FAIL: no trace $Trace"; exit 1 }
if ($OutDir -match " ") { Write-Host "FAIL: -OutDir must not contain spaces"; exit 1 }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Stem = if ($Name) { $Name } else { "$Label.$Command" }
$Out = "$OutDir\$Stem.csv"
$Cmd = "TimingInsights.Export$Command $Out $Extra".Trim()
$p = Start-Process -FilePath $Insights -ArgumentList @("-OpenTraceFile=`"$Trace`"", "-AutoQuit", "-NoUI", "-log", "-ABSLOG=`"$OutDir\$Stem.log`"", "-ExecOnAnalysisCompleteCmd=`"$Cmd`"") -PassThru -Wait -NoNewWindow
Write-Host "exit=$($p.ExitCode) out=$Out"
exit $p.ExitCode
