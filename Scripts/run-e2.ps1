# M2 실험 E2 묶음. Test 패키지로 조건을 연달아 잰다. plan.md "M2".
#   (a) M1(애니 없는 ISM, 변형 noanim, -Stage M1) vs M2(VAT ISM, 변형 mass): N {1k, 5k, 20k, 50k} x P1, P2 x 3회.
#       같은 묶음에서 N·프리셋마다 M1 → M2 순으로 번갈아 잰다(measurement.md "같은 날 연달아 잰 묶음끼리만 비교"). 결과는 모두 Measurements/M2/.
#   (b) vatvert (c) vatcull, vatcullvis (d) vatdie0, vatdie1k: N=5k x P1 x 3회(하위 실험은 앵커 조건만, measurement.md).
# 묶음의 맨 첫 조건만 r0(버리는 실행)을 둔다. 총 21조건 63회 + r0, 실행 하나 약 85초(약 90분).
param(
    [string]$Tag = "",
    [switch]$OnlySub
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

$Conditions = @()
if (-not $OnlySub) {
    foreach ($N in @(1000, 5000, 20000, 50000)) {
        foreach ($Preset in @("P1", "P2")) {
            $Conditions += , @("noanim", $N, $Preset, "M1")
            $Conditions += , @("mass", $N, $Preset, "")
        }
    }
}
foreach ($V in @("vatvert", "vatcull", "vatcullvis", "vatdie0", "vatdie1k")) { $Conditions += , @($V, 5000, "P1", "") }

$Start = Get-Date
$Failed = @()
$First = $true
foreach ($C in $Conditions) {
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M2", "-Variant", $C[0], "-N", $C[1], "-Preset", $C[2])
    if ($C[3]) { $Args += @("-Stage", $C[3]) }
    if ($Tag) { $Args += @("-Tag", $Tag) }
    if (-not $First) { $Args += "-SkipR0" }
    $First = $false
    & powershell @Args
    if ($LASTEXITCODE -ne 0) { $Failed += "$($C[0]) N$($C[1]) $($C[2])" }
}
$Elapsed = (Get-Date) - $Start
Write-Host ("E2 DONE: {0} conditions, {1:hh\:mm\:ss}, failed={2}" -f $Conditions.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
