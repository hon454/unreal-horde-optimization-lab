# M5 고정 바운드 실험 러너(Worklog/M5.md "측정 세션"). 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안 PC를 쓰지 않는다).
# 20k P1, 2종 -Types 2, 라벨 접두사 M5-. 순서:
#   r0     : m5iskm 버리는 실행 1회, 태그 bndwarm
#   main   : m4offpool(-LabStage=M4), m5iskm, m5bounds(-LabStage=M5)를 3회씩 실행 단위로 섞는다(A r1, B r1, C r1, A r2, ...), 태그 bnd
#            summary에서는 <변형>-bnd 조건으로 따로 요약된다(42회 묶음의 행은 바뀌지 않는다)
#   trace  : m5bounds 병렬 1회(tracepar), 직렬 1회(traceser, -LabFullyParallel=0). 짝은 M5-m5iskm-N20000-P1tracepar-r1 / traceser-r1. 트레이스 수치는 비교에 쓰지 않는다
# 합계 1 + 9 + 2 = 12회. 메타데이터 검사는 run-m5-all.ps1과 같다(M5 단계는 Lab/IskmInstances 중앙값 > 0).
# 실행 실패(종료 코드)는 같은 라벨로 1회 재시도하고(트레이스는 번호를 올려 r2), 또 실패하거나 메타데이터 검사가 어긋나면 멈춘다.
# 기록 Measurements/M5/bounds-runs.csv, 로그 Saved/Lab/m5-bounds.log.
# 끝나면 summarize.py와 spikes.py를 돌려 Measurements/M5/{runs.csv,summary.csv,spikes.txt}를 갱신한다. 커밋은 하지 않는다. -DryRun은 목록만 찍는다.
param([switch]$DryRun, [switch]$SkipPreflight, [string]$SerialArgs = "-LabFullyParallel=0")
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRoot\run-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$LogDir = "$Root\Saved\Lab"; New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$Log = "$LogDir\m5-bounds.log"
$MeasDir = "$Root\Measurements\M5"
$RunCsv = "$MeasDir\bounds-runs.csv"

function Add-Log($Text) {
    try {
        $fs = New-Object System.IO.FileStream($Log, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite)
        $w = New-Object System.IO.StreamWriter($fs, (New-Object System.Text.UTF8Encoding($false)))
        $w.WriteLine($Text); $w.Close()
    } catch { }
}
function Say($m) { $l = "{0:yyyy-MM-dd HH:mm:ss} {1}" -f (Get-Date), $m; Write-Host $l; if (-not $DryRun) { Add-Log $l } }

$StageOf = @{ m4offpool = "M4"; m5iskm = "M5"; m5bounds = "M5" }
$Runs = New-Object System.Collections.ArrayList
function Add-Run($Variant, $RunIdx, $RunTag, $Trace = $false, $Extra = "") {
    [void]$Runs.Add(@{ Variant = $Variant; N = 20000; Preset = "P1"; Stage = $StageOf[$Variant]; Run = $RunIdx; Tag = $RunTag; Trace = $Trace; Extra = $Extra })
}
Add-Run "m5iskm" 0 "bndwarm"                                             # r0 (버린다)
foreach ($R in 1..3) { foreach ($V in @("m4offpool", "m5iskm", "m5bounds")) { Add-Run $V $R "bnd" } }
Add-Run "m5bounds" 1 "tracepar" $true
Add-Run "m5bounds" 1 "traceser" $true $SerialArgs

$Plan = $Runs.ToArray()
Write-Host "M5 bounds batch: $($Plan.Count) runs (incl. 1 discarded r0), about $([math]::Round($Plan.Count * 82 / 60)) min (traces take longer)"
$Idx = 0
foreach ($R in $Plan) {
    $Idx++
    Write-Host ("{0,3}. M5-{1}-N{2}-{3}{4}-r{5}  stage={6}{7}{8}" -f $Idx, $R.Variant, $R.N, $R.Preset, $R.Tag, $R.Run, $R.Stage, $(if ($R.Trace) { " trace" } else { "" }), $(if ($R.Extra) { " extra=$($R.Extra)" } else { "" }))
}
if ($DryRun) { Write-Host "DryRun: nothing executed."; exit 0 }

if (-not $SkipPreflight) {
    $Bad = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" } | ForEach-Object { $_.ProcessName })
    if ($Bad.Count -gt 0) { Say "FAIL preflight: running $($Bad -join ',')"; exit 2 }
    if (-not (Test-Path "$Root\Packaged\Test\Windows\HordeOptLab\Binaries\Win64\HordeOptLab-Win64-Test.exe")) { Say "FAIL preflight: no Test package"; exit 2 }
}
if (Test-Path $RunCsv) { Say "FAIL: $RunCsv already exists. 옮기거나 지운 뒤 다시 돌린다."; exit 2 }
New-Item -ItemType Directory -Force -Path $MeasDir | Out-Null
Set-Content -Path $RunCsv -Value "label,start,end,seconds,attempts,exit_code,check" -Encoding UTF8
$Commit = (Get-Content (Get-ChildItem -Recurse -Path "$Root\Packaged\Test" -Filter commit.txt | Select-Object -First 1).FullName -Raw).Trim()
Say "M5 BOUNDS start. package=$Commit head=$(git rev-parse --short HEAD) runs=$($Plan.Count)"

# 실행 뒤 CSV 메타데이터 검사. 통과하면 "ok", 아니면 사유.
function Test-Meta($Label, $Stage) {
    $p = "$OutCsv\$Label.csv"
    if (-not (Test-Path $p)) { return "no csv" }
    $Text = (Get-Content $p -Tail 3) -join "`n"
    $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
    $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value
    if ($Types -ne "2") { return "labtypes=$Types (expected 2)" }
    if (($Stage -eq "M4" -or $Stage -eq "M5") -and ($Lod -eq "" -or $Lod -eq "-")) { return "labsimlod=$Lod on an $Stage run" }
    if ($Stage -eq "M5") {
        $Out = (& py -3 -I "$PSScriptRoot\check-iskm.py" $p 2>&1 | Out-String).Trim()
        if ($LASTEXITCODE -ne 0) { return "IskmInstances median=$Out on an M5 run (ISKM not drawn)" }
    }
    return "ok"
}

$Failed = $false
foreach ($R in $Plan) {
    $RunIdx = $R.Run
    $T0 = Get-Date; $Attempts = 0; $Code = 1; $Check = "run failed"; $Label = ""
    foreach ($Try in 1, 2) {
        $Attempts = $Try
        # 트레이스는 라벨을 재사용하지 않고 번호를 올려 다시 돈다. 그 밖에는 같은 라벨(실패한 실행은 CSV를 남기지 않는다)
        if ($R.Trace -and $Try -eq 2) { $RunIdx = $R.Run + 1 }
        $Label = "M5-$($R.Variant)-N$($R.N)-$($R.Preset)$($R.Tag)-r$RunIdx"
        $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M5", "-Variant", $R.Variant, "-N", $R.N, "-Preset", $R.Preset,
                  "-Stage", $R.Stage, "-RunIndex", $RunIdx, "-Types", 2, "-Tag", $R.Tag)
        if ($R.Trace) { $Args += "-Trace" }
        if ($R.Extra) { $Args += @("-ExtraArgs", $R.Extra) }
        if ($Try -eq 2) { Say "RETRY once: $Label" }
        Say "RUN $Label"
        & powershell @Args 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
        $Code = $LASTEXITCODE
        $Check = if ($Code -eq 0) { Test-Meta $Label $R.Stage } else { "run failed" }
        if ($Code -eq 0) { break }   # 실행은 됐다. 검사가 어긋나면 재시도 없이 아래에서 멈춘다
    }
    $T1 = Get-Date
    Add-Content -Path $RunCsv -Value ("{0},{1:s},{2:s},{3},{4},{5},{6}" -f $Label, $T0, $T1, [int]($T1 - $T0).TotalSeconds, $Attempts, $Code, $Check) -Encoding UTF8
    if ($Code -ne 0 -or $Check -ne "ok") { Say "STOP: $Label code=$Code check=$Check"; $Failed = $true; break }
}

Say "summarize"
& py -3 -I "$PSScriptRoot\summarize.py" --milestone M5 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
Say "spikes"
& py -3 -I "$PSScriptRoot\spikes.py" "M5-" --verbose 2>&1 | Out-File -FilePath "$MeasDir\spikes.txt" -Encoding UTF8
Say "M5 BOUNDS done. failed=$Failed. 커밋은 하지 않았다(Measurements/M5/)."
exit $(if ($Failed) { 1 } else { 0 })
