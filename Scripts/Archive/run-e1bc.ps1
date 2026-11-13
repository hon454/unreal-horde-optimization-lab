# M1 실험 E1 (b)(c) 묶음. Test 패키지. plan.md "M1".
#   (b) massst: mass.FullyParallel 0. (c) massav: 엔진 회피 켬. 비교 대상은 E1 (a)의 mass.
#   N {5k, 20k, 50k} x P1 x 3회. 병렬·회피 비용은 N이 클 때 드러나므로 큰 N만 잰다.
# 묶음의 맨 첫 조건만 r0(버리는 실행)을 둔다. 총 19회, 실행 하나 약 85초.
param(
    [string]$Tag = ""
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

$Conditions = @()
foreach ($Variant in @("massst", "massav")) {
    foreach ($N in @(5000, 20000, 50000)) { $Conditions += , @($Variant, $N, "P1") }
}

$Start = Get-Date
$Failed = @()
$First = $true
foreach ($C in $Conditions) {
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M1", "-Variant", $C[0], "-N", $C[1], "-Preset", $C[2])
    if ($Tag) { $Args += @("-Tag", $Tag) }
    if (-not $First) { $Args += "-SkipR0" }
    $First = $false
    & powershell @Args
    if ($LASTEXITCODE -ne 0) { $Failed += "$($C[0]) N$($C[1]) $($C[2])" }
}
$Elapsed = (Get-Date) - $Start
Write-Host ("E1bc DONE: {0} conditions, {1:hh\:mm\:ss}, failed={2}" -f $Conditions.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
