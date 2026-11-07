# M1 실험 E1 (a) 묶음: 컨테이너 전환(액터 → Mass). Test 패키지로 조건을 연달아 잰다. plan.md "M1".
#   mass: N {1k, 2k, 5k, 20k, 50k} x P1, P2 x 3회. 1k·2k·5k는 E0 static과 겹치는 비교점이고 20k·50k는 M1의 한계를 찾는 점이다.
# 묶음의 맨 첫 조건만 r0(버리는 실행)을 둔다. 총 31회, 실행 하나 약 85초.
# 프레임이 100ms를 넘는 조건은 엔진 이동이 시간을 0.1초로 잘라 시나리오가 달라진다(engine-notes.md 11절). 그 조건은 "참고"로 적는다.
# (b) mass.FullyParallel, (c) 회피, (d) 피해 전달은 따로 더한다.
param(
    [string]$Tag = ""
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

$Conditions = @()
foreach ($N in @(1000, 2000, 5000, 20000, 50000)) {
    foreach ($Preset in @("P1", "P2")) { $Conditions += , @("mass", $N, $Preset) }
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
Write-Host ("E1a DONE: {0} conditions, {1:hh\:mm\:ss}, failed={2}" -f $Conditions.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
