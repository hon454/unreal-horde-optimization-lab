# M3b 묶음 1·2 전체 러너. 한 번에 순서대로 돈다(같은 날 연달아가 규칙). 사용자가 "시작"을 정한 뒤에만 띄운다.
# 순서: start(r0 + 시작 A/A 3) -> e3b 24 -> matrix 120 -> ab2 9 -> aa 3 -> trace 2 (묶음 1) -> types2 132 (묶음 2). r0은 맨 앞 한 번만(전체 294행).
# 실행이 하나라도 실패하면 거기서 멈추고 로그에 남긴다(이어서 돌지 않는다). 끝나면 spikes.py와 summarize.py를 돌리고
# Measurements/M3b/ 에 batch-phases.csv, spikes.txt, batch-report.md 를 쓰고 Docs/Worklog/M3b.md "묶음 1·2 결과" 절을 만든다.
# 커밋은 하지 않는다. 로그: Saved/Lab/m3b-all.log. -DryRun은 각 페이즈의 -DryRun만 돌려 행 수를 합산한다.
# 모든 라벨에 태그 fg를 붙인다(옛 fin·prelude 라벨과 겹치지 않게. 예 M3b-m3mass-N20000-P1startfg-r0, ...P1t2fg-r1). summary 조건은 <변형>[-접미사]-fg, runs·summary의 final=1. 옛 fin 행은 final=0(2026-11-10, 포그라운드 문제로 대체). run-bench가 포그라운드를 검사하고 run-m3b가 실패 실행을 1회 재시도한다.
param([switch]$DryRun, [switch]$SkipPreflight)
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Runner = "$PSScriptRoot\run-m3b.ps1"
$LogDir = "$Root\Saved\Lab"; New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$Log = "$LogDir\m3b-all.log"
$PhaseCsv = "$Root\Measurements\M3b\batch-phases.csv"
# 로그 쓰기는 다른 프로세스가 읽는 중이어도 실패하지 않게 FileShare.ReadWrite로 열고, 실패하면 무시한다(측정을 멈추지 않는다).
function Add-Log($Text) {
    try {
        $fs = New-Object System.IO.FileStream($Log, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite)
        $w = New-Object System.IO.StreamWriter($fs, (New-Object System.Text.UTF8Encoding($false)))
        $w.WriteLine($Text); $w.Close()
    } catch { }
}
function Say($m) { $l = "{0:yyyy-MM-dd HH:mm:ss} {1}" -f (Get-Date), $m; Write-Host $l; if (-not $DryRun) { Add-Log $l } }

# 묶음 1(start r0 포함 1회) -> 묶음 2. 첫 항목만 r0을 둔다.
$Phases = @(
    @{ Name = "start"; Args = @() },
    @{ Name = "e3b"; Args = @("-NoR0") },
    @{ Name = "matrix"; Args = @("-NoR0") },
    @{ Name = "ab2"; Args = @("-NoR0") },
    @{ Name = "aa"; Args = @("-NoR0") },
    @{ Name = "trace"; Args = @("-NoR0") },
    @{ Name = "types2"; Args = @("-NoR0") }
)

if ($DryRun) {
    $Sum = 0
    foreach ($P in $Phases) {
        $Out = & powershell -NoProfile -ExecutionPolicy Bypass -File $Runner -Phase $P.Name @($P.Args) -Tag fg -DryRun
        $Line = $Out | Where-Object { $_ -match "^M3b batch:" } | Select-Object -First 1
        $N = [int]([regex]::Match($Line, "= (\d+),").Groups[1].Value)
        if (-not $N) { $N = [int]([regex]::Match($Line, "= (\d+)").Groups[1].Value) }
        $Sum += $N
        Write-Host ("{0,-7} {1,4} runs   {2}" -f $P.Name, $N, $Line)
    }
    Write-Host "TOTAL $Sum runs (r0 incl.), 85 s each = $([math]::Round($Sum * 85 / 3600, 2)) h, 80 s each = $([math]::Round($Sum * 80 / 3600, 2)) h"
    exit 0
}

# --- 사전 점검(점검표 1·2번) ---
if (-not $SkipPreflight) {
    $Bad = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" } | ForEach-Object { $_.ProcessName })
    if ($Bad.Count -gt 0) { Say "FAIL preflight: running $($Bad -join ',')"; exit 2 }
    if (-not (Test-Path "$Root\Packaged\Test\Windows\HordeOptLab\Binaries\Win64\HordeOptLab-Win64-Test.exe")) { Say "FAIL preflight: no Test package"; exit 2 }
    $Dirty = @(git status --short --untracked-files=no)
    if ($Dirty.Count -gt 0) { Say "FAIL preflight: git status not clean: $($Dirty -join ' | ')"; exit 2 }
}
if (Test-Path $PhaseCsv) { Say "FAIL: $PhaseCsv already exists (이전 전체 실행 기록). 옮기거나 지운 뒤 다시 돌린다."; exit 2 }
New-Item -ItemType Directory -Force -Path (Split-Path $PhaseCsv) | Out-Null
Set-Content -Path $PhaseCsv -Value "phase,start,end,seconds,exit_code" -Encoding UTF8
$Commit = (Get-Content (Get-ChildItem -Recurse -Path "$Root\Packaged\Test" -Filter commit.txt | Select-Object -First 1).FullName -Raw).Trim()
Say "M3b ALL start. package=$Commit head=$(git rev-parse --short HEAD)"

$Failed = $false
foreach ($P in $Phases) {
    $T0 = Get-Date
    Say "PHASE $($P.Name) start"
    & powershell -NoProfile -ExecutionPolicy Bypass -File $Runner -Phase $P.Name @($P.Args) -Tag fg 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
    $Code = $LASTEXITCODE
    $T1 = Get-Date
    Add-Content -Path $PhaseCsv -Value ("{0},{1:s},{2:s},{3},{4}" -f $P.Name, $T0, $T1, [int]($T1 - $T0).TotalSeconds, $Code) -Encoding UTF8
    Say "PHASE $($P.Name) end exit=$Code"
    if ($Code -ne 0) { Say "STOP: phase $($P.Name) failed (exit $Code). 이후 페이즈는 돌리지 않는다."; $Failed = $true; break }
}

# --- 마무리: 실패해도 지금까지의 결과를 요약한다 ---
Say "summarize"
& py -3 -I "$PSScriptRoot\summarize.py" --milestone M3b 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
Say "spikes"
& py -3 -I "$PSScriptRoot\spikes.py" "M3b-" --verbose 2>&1 | Out-File -FilePath "$Root\Measurements\M3b\spikes.txt" -Encoding UTF8
& py -3 -I "$PSScriptRoot\m3b-report.py" --failed $(if ($Failed) { 1 } else { 0 }) 2>&1 | ForEach-Object { Add-Log $_; Write-Host $_ }
Say "M3b ALL done. failed=$Failed. 커밋은 하지 않았다(Measurements/M3b/, Docs/Worklog/M3b.md)."
exit $(if ($Failed) { 1 } else { 0 })
