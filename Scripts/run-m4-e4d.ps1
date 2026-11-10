# E4 (d) 묶음 러너(Worklog/M4.md "측정 세션: E4 (d) 묶음"). 한 번에 순서대로 돈다. 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안 PC를 쓰지 않는다).
# 변형: m3mass(-LabStage=M3, 기준점), m4mass(M4 대표), m4offmove(M4, 200m 밖을 Simulation LOD의 Off 밴드로 두고 Lab 프로세서가 옮긴다). 모두 2종(-Types 2).
# 라벨 접두사 M4-, 태그 e4d(기존 M4 묶음의 같은 조건 라벨과 겹치지 않는다. summary 조건은 <변형>-e4d).
#   r0     : m4mass 20k P1 버리는 실행(태그 e4dwarm)
#   start  : m4mass 20k P1 3회, 태그 e4dstart (시작 A/A)
#   main   : 조건 P1 10k, P1 30k, P2 20k, P1 20k를 차례로. 조건마다 실행 단위로 섞는다(m3mass r1, m4mass r1, m4offmove r1, m3mass r2, ...)
#   aa     : m4mass 20k P1 3회, 태그 e4daa (끝 A/A)
#   trace  : 20k P1 직렬 트레이스(-LabFullyParallel=0) m4mass, m4offmove (태그 traceser2), P2 병렬 트레이스 m4mass, m4offmove (태그 tracepard). -Trace 수치는 비교에 쓰지 않는다
# 합계 1 + 3 + 36 + 3 + 4 = 47회. 실행마다 CSV 메타데이터 검사: labtypes=2, labsimlod는 m3mass면 `-`, m4offmove면 ` offmove=1`을 포함, m4mass면 `-`가 아니고 offmove가 없다.
# 실패는 한 번 재시도(라벨을 올려 r+10). 또 실패하면 멈춘다. 기록: Measurements/M4/batch-runs-e4d.csv, 로그 Saved/Lab/m4-e4d.log. 끝나면 summarize.py와 spikes.py. 커밋은 하지 않는다.
param([switch]$DryRun, [switch]$SkipPreflight, [string]$SerialArgs = "-LabFullyParallel=0")
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRoot\run-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$LogDir = "$Root\Saved\Lab"; New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$Log = "$LogDir\m4-e4d.log"
$MeasDir = "$Root\Measurements\M4"
$RunCsv = "$MeasDir\batch-runs-e4d.csv"
function Add-Log($Text) { try { $fs = New-Object System.IO.FileStream($Log, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite); $w = New-Object System.IO.StreamWriter($fs, (New-Object System.Text.UTF8Encoding($false))); $w.WriteLine($Text); $w.Close() } catch { } }
function Say($m) { $l = "{0:yyyy-MM-dd HH:mm:ss} {1}" -f (Get-Date), $m; Write-Host $l; if (-not $DryRun) { Add-Log $l } }
$StageOf = @{ m3mass = "M3"; m4mass = "M4"; m4offmove = "M4" }
$Runs = New-Object System.Collections.ArrayList
function Add-Run($Variant, $N, $Preset, $RunIdx, $RunTag, $Trace = $false, $Extra = "") {
    [void]$Runs.Add(@{ Variant = $Variant; N = $N; Preset = $Preset; Stage = $StageOf[$Variant]; Run = $RunIdx; Tag = $RunTag; Trace = $Trace; Extra = $Extra })
}
Add-Run "m4mass" 20000 "P1" 0 "e4dwarm"
foreach ($R in 1..3) { Add-Run "m4mass" 20000 "P1" $R "e4dstart" }
foreach ($C in @(@(10000, "P1"), @(30000, "P1"), @(20000, "P2"), @(20000, "P1"))) {
    foreach ($R in 1..3) { foreach ($V in @("m3mass", "m4mass", "m4offmove")) { Add-Run $V $C[0] $C[1] $R "e4d" } }
}
foreach ($R in 1..3) { Add-Run "m4mass" 20000 "P1" $R "e4daa" }
Add-Run "m4mass" 20000 "P1" 1 "traceser2" $true $SerialArgs
Add-Run "m4offmove" 20000 "P1" 1 "traceser2" $true $SerialArgs
Add-Run "m4mass" 20000 "P2" 1 "tracepard" $true
Add-Run "m4offmove" 20000 "P2" 1 "tracepard" $true
$Plan = $Runs.ToArray()
Write-Host "E4 (d) batch: $($Plan.Count) runs (incl. 1 discarded r0), about $([math]::Round($Plan.Count * 82 / 60)) min"
$Idx = 0
foreach ($R in $Plan) { $Idx++; Write-Host ("{0,3}. M4-{1}-N{2}-{3}{4}-r{5}  stage={6}{7}{8}" -f $Idx, $R.Variant, $R.N, $R.Preset, $R.Tag, $R.Run, $R.Stage, $(if ($R.Trace) { " trace" } else { "" }), $(if ($R.Extra) { " extra=$($R.Extra)" } else { "" })) }
if ($DryRun) { Write-Host "DryRun: nothing executed."; exit 0 }
if (-not $SkipPreflight) {
    $Bad = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" } | ForEach-Object { $_.ProcessName })
    if ($Bad.Count -gt 0) { Say "FAIL preflight: running $($Bad -join ',')"; exit 2 }
    if (-not (Test-Path "$Root\Packaged\Test\Windows\HordeOptLab\Binaries\Win64\HordeOptLab-Win64-Test.exe")) { Say "FAIL preflight: no Test package"; exit 2 }
}
if (Test-Path $RunCsv) { Say "FAIL: $RunCsv already exists."; exit 2 }
New-Item -ItemType Directory -Force -Path $MeasDir | Out-Null
Set-Content -Path $RunCsv -Value "label,start,end,seconds,attempts,exit_code,check" -Encoding UTF8
$Commit = (Get-Content (Get-ChildItem -Recurse -Path "$Root\Packaged\Test" -Filter commit.txt | Select-Object -First 1).FullName -Raw).Trim()
Say "E4d ALL start. package=$Commit head=$(git rev-parse --short HEAD) runs=$($Plan.Count)"
function Test-Meta($Label, $Variant) {
    $p = "$OutCsv\$Label.csv"
    if (-not (Test-Path $p)) { return "no csv" }
    $Text = (Get-Content $p -Tail 3) -join "`n"
    $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
    $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value
    $Lod = $Lod.Trim()
    if ($Types -ne "2") { return "labtypes=$Types (expected 2)" }
    if ($Variant -eq "m3mass" -and $Lod -ne "-") { return "labsimlod=$Lod on m3mass" }
    if ($Variant -eq "m4mass" -and ($Lod -eq "" -or $Lod -eq "-" -or $Lod -like "*offmove*")) { return "labsimlod=$Lod on m4mass" }
    if ($Variant -eq "m4offmove" -and $Lod -notlike "*offmove=1*") { return "labsimlod=$Lod on m4offmove" }
    return "ok"
}
$Failed = $false
foreach ($R in $Plan) {
    $Attempts = 0; $Code = 1; $Check = "not run"; $UseIdx = $R.Run
    $T0 = Get-Date
    foreach ($Try in 1, 2) {
        $Attempts = $Try
        if ($Try -eq 2) { $UseIdx = $R.Run + 10 }
        $Label = "M4-$($R.Variant)-N$($R.N)-$($R.Preset)$($R.Tag)-r$UseIdx"
        $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M4", "-Variant", $R.Variant, "-N", $R.N, "-Preset", $R.Preset, "-Stage", $R.Stage, "-RunIndex", $UseIdx, "-Types", 2, "-Tag", $R.Tag)
        if ($R.Trace) { $Args += "-Trace" }
        if ($R.Extra) { $Args += @("-ExtraArgs", $R.Extra) }
        Say "RUN $Label"
        & powershell @Args 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
        $Code = $LASTEXITCODE
        $Check = if ($Code -eq 0) { Test-Meta $Label $R.Variant } else { "run failed" }
        if ($Code -eq 0 -and $Check -eq "ok") { break }
        Say "RETRY: $Label code=$Code check=$Check"
    }
    $T1 = Get-Date
    Add-Content -Path $RunCsv -Value ("{0},{1:s},{2:s},{3},{4},{5},{6}" -f $Label, $T0, $T1, [int]($T1 - $T0).TotalSeconds, $Attempts, $Code, $Check) -Encoding UTF8
    if ($Code -ne 0 -or $Check -ne "ok") { Say "STOP: $Label code=$Code check=$Check"; $Failed = $true; break }
}
Say "summarize"
& py -3 -I "$PSScriptRoot\summarize.py" --milestone M4 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
Say "spikes"
& py -3 -I "$PSScriptRoot\spikes.py" "M4-" --verbose 2>&1 | Out-File -FilePath "$MeasDir\spikes-e4d.txt" -Encoding UTF8
Say "E4d ALL done. failed=$Failed. 커밋은 하지 않았다."
exit $(if ($Failed) { 1 } else { 0 })
