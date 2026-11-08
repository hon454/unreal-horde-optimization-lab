# M3 실험 E3 묶음. Test 패키지로 조건을 연달아 잰다. plan.md "M3".
#   (1) 대표: M2 + 200m 밖 Off(변형 m2off, -Stage M2) vs M3 하이드레이션만(변형 mass): N {1k, 5k, 20k, 50k} x P1, P2 x 3회.
#       같은 묶음에서 N·프리셋마다 M2 → M3 순으로 번갈아 잰다. 결과는 모두 Measurements/M3/.
#   (2) m3uro (3a) m3budgetdist (3b) m3budgetsig (5) m3lowres: N=5k x P1 x 3회(하위 실험은 앵커 조건만).
# 묶음의 맨 첫 조건만 r0(버리는 실행)을 둔다. 총 20조건 60회 + r0, 실행 하나 약 85초(약 85분).
param(
    [string]$Tag = "",
    [switch]$OnlySub,
    [switch]$OnlyMain,  # (1) 대표만. 결과의 비용 내역을 보고 하위 실험을 고른다(2026-11-08 사용자 결정)
    [switch]$OnlyDistance,  # (7) 하이드레이션 거리만: m3hydbase(= mass, 30m), m3hyd15, m3hyd50 x {5k, 20k} P1(2026-11-09 사용자 결정)
    [switch]$OnlyCollision  # (6) 콜리전 줄이기만: m3colbase(= mass), m3colquery, m3nocol x {5k, 20k} P1(2026-11-09 사용자 결정)
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

$Conditions = @()
if (-not $OnlySub) {
    foreach ($N in @(1000, 5000, 20000, 50000)) {
        foreach ($Preset in @("P1", "P2")) {
            $Conditions += , @("m2off", $N, $Preset, "M2")
            $Conditions += , @("mass", $N, $Preset, "")
        }
    }
}
# 하위 실험은 같은 묶음 안에서 비교하도록 대표(m3base = mass와 같음)를 맨 앞에 다시 잰다(2026-11-09).
if ($OnlyDistance) {
    foreach ($N in @(5000, 20000)) { foreach ($V in @("m3hydbase", "m3hyd15", "m3hyd50")) { $Conditions += , @($V, $N, "P1", "") } }
}
elseif ($OnlyCollision) {
    foreach ($N in @(5000, 20000)) { foreach ($V in @("m3colbase", "m3colquery", "m3nocol")) { $Conditions += , @($V, $N, "P1", "") } }
}
elseif (-not $OnlyMain) { foreach ($V in @("m3base", "m3uro", "m3budgetdist", "m3budgetsig", "m3lowres")) { $Conditions += , @($V, 5000, "P1", "") } }

$Start = Get-Date
$Failed = @()
$First = $true
foreach ($C in $Conditions) {
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M3", "-Variant", $C[0], "-N", $C[1], "-Preset", $C[2])
    if ($C[3]) { $Args += @("-Stage", $C[3]) }
    if ($Tag) { $Args += @("-Tag", $Tag) }
    if (-not $First) { $Args += "-SkipR0" }
    $First = $false
    & powershell @Args
    if ($LASTEXITCODE -ne 0) { $Failed += "$($C[0]) N$($C[1]) $($C[2])" }
}
$Elapsed = (Get-Date) - $Start
Write-Host ("E3 DONE: {0} conditions, {1:hh\:mm\:ss}, failed={2}" -f $Conditions.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
