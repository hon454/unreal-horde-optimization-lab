# "그리는 거리" 묶음 러너(Worklog/M4.md "측정 세션: 그리는 거리 묶음"). m4offmove(M4, 2종) P1에서 -LabFarM 200/300/500을 잰다. 사용자가 시작을 정한 뒤에만 띄운다.
#   r0 : far200 20k 버리는 실행(태그 farwarm). 이어 N 20,000 -> N 30,000 각각 far200·300·500을 번갈아 3회(태그 far200/far300/far500). 합계 1 + 9 + 9 = 19회.
# 실행마다 CSV 메타데이터 검사: labtypes=2, labsimlod에 offmove=1, labfarm = 요청값. 실패는 한 번 재시도(r+10), 또 실패하면 멈춘다. 기록 Measurements/M4/batch-runs-far.csv. 커밋은 하지 않는다.
param([switch]$DryRun, [switch]$SkipPreflight, [int]$Ns30 = 1)
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRootun-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$LogDir = "$Root\Saved\Lab"; New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$Log = "$LogDir\m4-far.log"
$MeasDir = "$Root\Measurements\M4"
$RunCsv = "$MeasDir\batch-runs-far.csv"
function Add-Log($Text) { try { $fs = New-Object System.IO.FileStream($Log, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite); $w = New-Object System.IO.StreamWriter($fs, (New-Object System.Text.UTF8Encoding($false))); $w.WriteLine($Text); $w.Close() } catch { } }
function Say($m) { $l = "{0:yyyy-MM-dd HH:mm:ss} {1}" -f (Get-Date), $m; Write-Host $l; if (-not $DryRun) { Add-Log $l } }
$Runs = New-Object System.Collections.ArrayList
function Add-Run($Far, $N, $RunIdx, $RunTag) { [void]$Runs.Add(@{ Far = $Far; N = $N; Run = $RunIdx; Tag = $RunTag }) }
Add-Run 200 20000 0 "farwarm"
foreach ($N in @(20000, 30000)) {
    if ($N -eq 30000 -and $Ns30 -eq 0) { continue }
    foreach ($R in 1..3) { foreach ($F in @(200, 300, 500)) { Add-Run $F $N $R "far$F" } }
}
$Plan = $Runs.ToArray()
Write-Host "far batch: $($Plan.Count) runs (incl. 1 discarded r0), about $([math]::Round($Plan.Count * 82 / 60)) min"
foreach ($R in $Plan) { Write-Host ("M4-m4offmove-N{0}-P1{1}-r{2}  far={3}" -f $R.N, $R.Tag, $R.Run, $R.Far) }
if ($DryRun) { Write-Host "DryRun: nothing executed."; exit 0 }
if (-not $SkipPreflight) {
    $Bad = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" } | ForEach-Object { $_.ProcessName })
    if ($Bad.Count -gt 0) { Say "FAIL preflight: running $($Bad -join ',')"; exit 2 }
    if (-not (Test-Path "$Root\Packaged\Test\Windows\HordeOptLab\Binaries\Win64\HordeOptLab-Win64-Test.exe")) { Say "FAIL preflight: no Test package"; exit 2 }
}
if (Test-Path $RunCsv) { Say "FAIL: $RunCsv already exists."; exit 2 }
Set-Content -Path $RunCsv -Value "label,start,end,seconds,attempts,exit_code,check" -Encoding UTF8
$Commit = (Get-Content (Get-ChildItem -Recurse -Path "$Root\Packaged\Test" -Filter commit.txt | Select-Object -First 1).FullName -Raw).Trim()
Say "far ALL start. package=$Commit head=$(git rev-parse --short HEAD) runs=$($Plan.Count)"
function Test-Meta($Label, $Far) {
    $p = "$OutCsv\$Label.csv"
    if (-not (Test-Path $p)) { return "no csv" }
    $Text = (Get-Content $p -Tail 3) -join "`n"
    $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
    $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value.Trim()
    $FarM = [regex]::Match($Text, "\[labfarm\],([^,]*)").Groups[1].Value.Trim()
    if ($Types -ne "2") { return "labtypes=$Types (expected 2)" }
    if ($Lod -notlike "*offmove=1*") { return "labsimlod=$Lod (expected offmove=1, stage M4)" }
    if ($FarM -ne "$Far") { return "labfarm=$FarM (expected $Far)" }
    return "ok"
}
$Failed = $false
foreach ($R in $Plan) {
    $Attempts = 0; $Code = 1; $Check = "not run"; $UseIdx = $R.Run
    $T0 = Get-Date
    foreach ($Try in 1, 2) {
        $Attempts = $Try
        if ($Try -eq 2) { $UseIdx = $R.Run + 10 }
        $Label = "M4-m4offmove-N$($R.N)-P1$($R.Tag)-r$UseIdx"
        $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M4", "-Variant", "m4offmove", "-N", $R.N, "-Preset", "P1", "-Stage", "M4", "-RunIndex", $UseIdx, "-Types", 2, "-FarM", $R.Far, "-Tag", $R.Tag)
        Say "RUN $Label"
        & powershell @Args 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
        $Code = $LASTEXITCODE
        $Check = if ($Code -eq 0) { Test-Meta $Label $R.Far } else { "run failed" }
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
& py -3 -I "$PSScriptRoot\spikes.py" "M4-" --verbose 2>&1 | Out-File -FilePath "$MeasDir\spikes-far.txt" -Encoding UTF8
Say "far ALL done. failed=$Failed. 커밋은 하지 않았다."
exit $(if ($Failed) { 1 } else { 0 })
