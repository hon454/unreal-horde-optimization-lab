# M3b 묶음 러너(초안, 아직 실행하지 않는다). plan.md "M3b" 측정 방법, measurement.md "벤치마크 조건".
# 한 묶음 = 정비 기준선(묶음 1). 같은 빌드로 한 번에 연달아 돌린다. 결과는 모두 Measurements/M3b/ (라벨 접두사 M3b).
#   Phase e3b   : E3b (8) 표현 LOD 값. m3vis0 / m3vis15 / m3hys20 x {20k, 5k} P1 x 3회 = 18회를 실행 단위로 섞는다(r1 라운드: 6조건, r2, r3).
#                 결과로 대표값(Base, 히스테리시스)을 정한 뒤 이어서 전수를 돌린다. 기준점 m3mass를 같은 섞기에 넣는다(24회, 2026-11-09 코디네이터 결정). -NoBase면 18회.
#   Phase ab    : 선행 A/B(코드 정비 효과). m3mass / m3qold / m3syncall x 20k P1 x 3회 = 9회를 실행 단위로 섞는다. -LabStage=M3.
#                 e3b의 m3mass 20k와 라벨이 겹치지 않도록 태그 ab를 붙인다(summary에서 <변형>-ab 조건으로 따로 쓰임).
#   Phase matrix: 전수 매트릭스, 조건 단위. M0 {full, static} x {1k, 5k} x P1, P2 / staticpawn {1k, 5k} P1 /
#                 M1~M3 {1k, 5k, 20k, 30k, 50k} x P1, P2. 각 3회.
#   Phase aa    : 묶음 끝 기준점 A/A. m3mass 20k P1 3회, 태그 aa(summary에서 m3mass-aa 조건으로 따로 쓰임).
#   Phase trace : 20k P1 m3mass 트레이스 병렬 1 + 직렬 1(-Trace). 태그 tracepar / traceser. 직렬은 mass.FullyParallel=0.
# 묶음의 맨 첫 실행 1회(r0)만 드라이버 PSO 캐시용으로 버린다. 나머지는 모두 r1 이상.
# 변형 이름은 단계별로 갈려야 한다(라벨 M3b-<변형>-N..-P..가 겹치지 않도록): m0full m0static staticpawn m1mass m2mass m3mass.
# -Stage는 변형 이름이 아니라 실행 단계(-LabStage)다. 변형 이름과 -LabStage 매핑, 직렬 인자는 코드 세션이 Docs/Worklog/M3b.md에 정한다.
# 다음 묶음(묶음 2, 2종)은 -Tag로 라벨을 바꿔 같은 러너를 쓴다.
# -DryRun은 실행 목록만 찍고 아무것도 실행하지 않는다.
param(
    [string]$Tag = "",
    [ValidateSet("all", "e3b", "ab", "matrix", "aa", "trace")][string]$Phase = "all",
    [switch]$NoBase,
    [switch]$NoR0,   # 묶음의 버리는 r0을 두지 않는다(같은 묶음의 두 번째 이후 호출. 예: e3b 다음 ab)
    [switch]$DryRun,
    [string]$SerialArgs = "-LabFullyParallel=0"   # 직렬 트레이스용 게임 인자(코드 세션이 확정)
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

# 실행 하나 = @(변형, N, 프리셋, 실행 단계 -Stage, 실행 번호, 라벨 태그, 트레이스 여부, 추가 인자)
$Runs = New-Object System.Collections.ArrayList
function Add-Run($Variant, $N, $Preset, $Stage, $RunIdx, $RunTag, $Trace, $Extra) {
    [void]$Runs.Add(@{ Variant = $Variant; N = $N; Preset = $Preset; Stage = $Stage; Run = $RunIdx; Tag = "$RunTag$Tag"; Trace = $Trace; Extra = $Extra })
}

# --- Phase e3b: 실행 단위로 섞는다 (A r1, B r1, C r1, ... A r2, ...) ---
if ($Phase -in @("all", "e3b")) {
    $Variants = @("m3vis0", "m3vis15", "m3hys20")
    if (-not $NoBase) { $Variants = @("m3mass") + $Variants }
    foreach ($R in 1..3) {
        foreach ($N in @(20000, 5000)) {
            foreach ($V in $Variants) { Add-Run $V $N "P1" "M3" $R "" $false "" }
        }
    }
}

# --- Phase ab: 실행 단위로 섞는다 ---
if ($Phase -in @("all", "ab")) {
    foreach ($R in 1..3) { foreach ($V in @("m3mass", "m3qold", "m3syncall")) { Add-Run $V 20000 "P1" "M3" $R "ab" $false "" } }
}

# --- Phase matrix: 조건 단위, 3회 ---
if ($Phase -in @("all", "matrix")) {
    $Cond = @()
    foreach ($V in @("m0full", "m0static")) { foreach ($N in @(1000, 5000)) { foreach ($P in @("P1", "P2")) { $Cond += , @($V, $N, $P, "M0") } } }
    foreach ($N in @(1000, 5000)) { $Cond += , @("staticpawn", $N, "P1", "M0") }
    foreach ($S in @(@("m1mass", "M1"), @("m2mass", "M2"), @("m3mass", "M3"))) {
        foreach ($N in @(1000, 5000, 20000, 30000, 50000)) { foreach ($P in @("P1", "P2")) { $Cond += , @($S[0], $N, $P, $S[1]) } }
    }
    foreach ($C in $Cond) { foreach ($R in 1..3) { Add-Run $C[0] $C[1] $C[2] $C[3] $R "" $false "" } }
}

# --- Phase aa: 묶음 끝 기준점 A/A ---
if ($Phase -in @("all", "aa")) {
    foreach ($R in 1..3) { Add-Run "m3mass" 20000 "P1" "M3" $R "aa" $false "" }
}

# --- Phase trace: 병렬 1 + 직렬 1 ---
if ($Phase -in @("all", "trace")) {
    Add-Run "m3mass" 20000 "P1" "M3" 1 "tracepar" $true ""
    Add-Run "m3mass" 20000 "P1" "M3" 1 "traceser" $true $SerialArgs
}

# 묶음의 맨 첫 실행은 r0(버리는 실행)로 한 번 더 돈다.
$Total = $Runs.Count + $(if ($NoR0) { 0 } else { 1 })
$Minutes = [math]::Round($Total * 85 / 60)
Write-Host "M3b batch: $($Runs.Count) runs + 1 discarded r0 = $Total, about $Minutes min (85 s each)"
$Idx = 0
$Plan = $Runs.ToArray()
if (-not $NoR0) { $Plan = @(@{ Variant = $Runs[0].Variant; N = $Runs[0].N; Preset = $Runs[0].Preset; Stage = $Runs[0].Stage; Run = 0; Tag = $Runs[0].Tag; Trace = $false; Extra = "" }) + $Plan }
foreach ($R in $Plan) {
    $Idx++
    $Label = "M3b-$($R.Variant)-N$($R.N)-$($R.Preset)$($R.Tag)-r$($R.Run)"
    Write-Host ("{0,3}. {1}  stage={2}{3}{4}" -f $Idx, $Label, $R.Stage, $(if ($R.Trace) { " trace" } else { "" }), $(if ($R.Extra) { " extra=$($R.Extra)" } else { "" }))
}
if ($DryRun) { Write-Host "DryRun: nothing executed."; exit 0 }

$Start = Get-Date
$Failed = @()
foreach ($R in $Plan) {
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M3b", "-Variant", $R.Variant, "-N", $R.N, "-Preset", $R.Preset,
              "-Stage", $R.Stage, "-RunIndex", $R.Run)
    if ($R.Tag) { $Args += @("-Tag", $R.Tag) }
    if ($R.Trace) { $Args += "-Trace" }
    if ($R.Extra) { $Args += @("-ExtraArgs", $R.Extra) }
    & powershell @Args
    if ($LASTEXITCODE -ne 0) { $Failed += "$($R.Variant) N$($R.N) $($R.Preset)$($R.Tag) r$($R.Run)" }
}
$Elapsed = (Get-Date) - $Start
Write-Host ("M3b DONE: {0} runs, {1:hh\:mm\:ss}, failed={2}" -f $Plan.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
Write-Host "Next: py -3 -I Scripts/summarize.py --milestone M3b ; py -3 -I Scripts/spikes.py M3b- --verbose"
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
