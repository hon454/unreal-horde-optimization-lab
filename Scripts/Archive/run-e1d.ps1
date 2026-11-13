# M1 실험 E1 (d) 피해 적용 방식 묶음. Test 패키지. plan.md "M1".
#   바로 깎기(massdir*) vs 피해 이벤트 엔티티로 모아 깎기(massevt*), 비치명 피해 초당 1천(lo)·1만(hi). 체력은 최소 1이라 사망 없음.
#   N 20k x P1 x 3회. 깎인 체력 때문에 범위 처치가 조금 빨라지는 것은 두 방식에 같다.
# 묶음의 맨 첫 조건만 r0(버리는 실행)을 둔다. 총 13회, 실행 하나 약 85초.
param(
    [string]$Tag = ""
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

$Conditions = @()
foreach ($Variant in @("massdirlo", "massevtlo", "massdirhi", "massevthi")) {
    $Conditions += , @($Variant, 20000, "P1")
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
Write-Host ("E1d DONE: {0} conditions, {1:hh\:mm\:ss}, failed={2}" -f $Conditions.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
