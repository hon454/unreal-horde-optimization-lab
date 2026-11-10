# M4 마일스톤 묶음 러너(ADR-0006 마일스톤 묶음). 한 번에 순서대로 돈다. 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안 PC를 쓰지 않는다).
# 순서(모두 2종 -Types 2, 라벨 접두사 M4-):
#   r0      : m4mass 20k P1 버리는 실행 1회(드라이버 PSO 캐시), 태그 warm. (첫 시도의 start-r0은 검사 코드 오류로 멈춰 이 라벨을 다시 쓰지 않으려고 태그를 바꿨다)
#   start   : m4mass 20k P1 3회, 태그 start (시작 A/A. summary 조건 m4mass-start)
#   main    : 조건 단위, 조건마다 기준점 m3mass(-LabStage=M3) 3회 -> m4mass(-LabStage=M4) 3회.
#             조건 = P1 10k, P1 30k, P2 20k, P1 20k (20k P1을 마지막에 두어 하위 실험 바로 앞에 둔다)
#   sub     : E4 하위 실험, 20k P1, 실행 단위로 섞는다(A r1, B r1, ..., A r2, ...): m4off m4m25l50 m4m25l100 m4m50l50 m4avoid. 앵커는 main의 m4mass 20k P1
#   aa      : m4mass 20k P1 3회, 태그 aa (끝 A/A. summary 조건 m4mass-aa)
#   trace   : 20k P1 트레이스(-Trace, 수치는 비교에 쓰지 않는다): m4mass 병렬, m4mass 직렬(-LabFullyParallel=0), m4off 직렬(E4 (a) 프로세서별 ms), 태그 tracepar / traceser
# 합계 1 + 3 + 24 + 15 + 3 + 3 = 49회.
# 실행마다 CSV 메타데이터를 검사한다: labtypes=2, labsimlod는 M4 변형이면 `-`가 아니고 m3mass면 `-`. 어긋나면 거기서 멈춘다.
# 실패(포그라운드 검사 FAIL 등)는 같은 라벨로 1회 재시도하고, 또 실패하면 멈춘다. 실행 기록은 Measurements/M4/batch-runs.csv, 로그는 Saved/Lab/m4-all.log.
# 끝나면 summarize.py와 spikes.py를 돌려 Measurements/M4/{runs.csv,summary.csv,spikes.txt}를 쓴다. 커밋은 하지 않는다. -DryRun은 목록만 찍는다.
param([switch]$DryRun, [switch]$SkipPreflight, [string]$SerialArgs = "-LabFullyParallel=0")
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRoot\run-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$LogDir = "$Root\Saved\Lab"; New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$Log = "$LogDir\m4-all.log"
$MeasDir = "$Root\Measurements\M4"
$RunCsv = "$MeasDir\batch-runs.csv"

function Add-Log($Text) {
    try {
        $fs = New-Object System.IO.FileStream($Log, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite)
        $w = New-Object System.IO.StreamWriter($fs, (New-Object System.Text.UTF8Encoding($false)))
        $w.WriteLine($Text); $w.Close()
    } catch { }
}
function Say($m) { $l = "{0:yyyy-MM-dd HH:mm:ss} {1}" -f (Get-Date), $m; Write-Host $l; if (-not $DryRun) { Add-Log $l } }

# 변형 -> 실행 단계(-LabStage). 변형 이름은 라벨일 뿐이고 단계는 이 표가 정한다.
$StageOf = @{ m3mass = "M3"; m4mass = "M4"; m4off = "M4"; m4m25l50 = "M4"; m4m25l100 = "M4"; m4m50l50 = "M4"; m4avoid = "M4" }
$Runs = New-Object System.Collections.ArrayList
function Add-Run($Variant, $N, $Preset, $RunIdx, $RunTag, $Trace = $false, $Extra = "") {
    [void]$Runs.Add(@{ Variant = $Variant; N = $N; Preset = $Preset; Stage = $StageOf[$Variant]; Run = $RunIdx; Tag = $RunTag; Trace = $Trace; Extra = $Extra })
}

Add-Run "m4mass" 20000 "P1" 0 "warm"                                    # r0 (버린다)
foreach ($R in 1..3) { Add-Run "m4mass" 20000 "P1" $R "start" }
foreach ($C in @(@(10000, "P1"), @(30000, "P1"), @(20000, "P2"), @(20000, "P1"))) {
    foreach ($V in @("m3mass", "m4mass")) { foreach ($R in 1..3) { Add-Run $V $C[0] $C[1] $R "" } }
}
foreach ($R in 1..3) { foreach ($V in @("m4off", "m4m25l50", "m4m25l100", "m4m50l50", "m4avoid")) { Add-Run $V 20000 "P1" $R "" } }
foreach ($R in 1..3) { Add-Run "m4mass" 20000 "P1" $R "aa" }
Add-Run "m4mass" 20000 "P1" 1 "tracepar" $true
Add-Run "m4mass" 20000 "P1" 1 "traceser" $true $SerialArgs
Add-Run "m4off" 20000 "P1" 1 "traceser" $true $SerialArgs

$Plan = $Runs.ToArray()
Write-Host "M4 batch: $($Plan.Count) runs (incl. 1 discarded r0), about $([math]::Round($Plan.Count * 82 / 60)) min"
$Idx = 0
foreach ($R in $Plan) {
    $Idx++
    $Label = "M4-$($R.Variant)-N$($R.N)-$($R.Preset)$($R.Tag)-r$($R.Run)"
    Write-Host ("{0,3}. {1}  stage={2}{3}{4}" -f $Idx, $Label, $R.Stage, $(if ($R.Trace) { " trace" } else { "" }), $(if ($R.Extra) { " extra=$($R.Extra)" } else { "" }))
}
if ($DryRun) { Write-Host "DryRun: nothing executed."; exit 0 }

if (-not $SkipPreflight) {
    $Bad = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" } | ForEach-Object { $_.ProcessName })
    if ($Bad.Count -gt 0) { Say "FAIL preflight: running $($Bad -join ',')"; exit 2 }
    if (-not (Test-Path "$Root\Packaged\Test\Windows\HordeOptLab\Binaries\Win64\HordeOptLab-Win64-Test.exe")) { Say "FAIL preflight: no Test package"; exit 2 }
}
if (Test-Path $RunCsv) { Say "FAIL: $RunCsv already exists (이전 전체 실행 기록). 옮기거나 지운 뒤 다시 돌린다."; exit 2 }
New-Item -ItemType Directory -Force -Path $MeasDir | Out-Null
Set-Content -Path $RunCsv -Value "label,start,end,seconds,attempts,exit_code,check" -Encoding UTF8
$Commit = (Get-Content (Get-ChildItem -Recurse -Path "$Root\Packaged\Test" -Filter commit.txt | Select-Object -First 1).FullName -Raw).Trim()
Say "M4 ALL start. package=$Commit head=$(git rev-parse --short HEAD) runs=$($Plan.Count)"

# 실행 뒤 CSV 메타데이터 검사. 통과하면 "ok", 아니면 사유.
function Test-Meta($Label, $Stage) {
    $p = "$OutCsv\$Label.csv"
    if (-not (Test-Path $p)) { return "no csv" }
    $Text = (Get-Content $p -Tail 3) -join "`n"
    $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
    $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value
    if ($Types -ne "2") { return "labtypes=$Types (expected 2)" }
    if ($Stage -eq "M4" -and ($Lod -eq "" -or $Lod -eq "-")) { return "labsimlod=$Lod on an M4 run" }
    if ($Stage -ne "M4" -and $Lod -ne "-") { return "labsimlod=$Lod on a non-M4 run" }
    return "ok"
}

$Failed = $false
foreach ($R in $Plan) {
    $Label = "M4-$($R.Variant)-N$($R.N)-$($R.Preset)$($R.Tag)-r$($R.Run)"
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M4", "-Variant", $R.Variant, "-N", $R.N, "-Preset", $R.Preset,
              "-Stage", $R.Stage, "-RunIndex", $R.Run, "-Types", 2)
    if ($R.Tag) { $Args += @("-Tag", $R.Tag) }
    if ($R.Trace) { $Args += "-Trace" }
    if ($R.Extra) { $Args += @("-ExtraArgs", $R.Extra) }
    $T0 = Get-Date; $Attempts = 1
    Say "RUN $Label"
    & powershell @Args 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
    $Code = $LASTEXITCODE
    if ($Code -ne 0) {
        Say "RETRY once: $Label (exit $Code)"
        $Attempts = 2
        & powershell @Args 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
        $Code = $LASTEXITCODE
    }
    $Check = if ($Code -eq 0) { Test-Meta $Label $R.Stage } else { "run failed" }
    $T1 = Get-Date
    Add-Content -Path $RunCsv -Value ("{0},{1:s},{2:s},{3},{4},{5},{6}" -f $Label, $T0, $T1, [int]($T1 - $T0).TotalSeconds, $Attempts, $Code, $Check) -Encoding UTF8
    if ($Code -ne 0 -or $Check -ne "ok") { Say "STOP: $Label code=$Code check=$Check"; $Failed = $true; break }
}

Say "summarize"
& py -3 -I "$PSScriptRoot\summarize.py" --milestone M4 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
Say "spikes"
& py -3 -I "$PSScriptRoot\spikes.py" "M4-" --verbose 2>&1 | Out-File -FilePath "$MeasDir\spikes.txt" -Encoding UTF8
Say "M4 ALL done. failed=$Failed. 커밋은 하지 않았다(Measurements/M4/, Docs/Worklog/M4.md)."
exit $(if ($Failed) { 1 } else { 0 })
