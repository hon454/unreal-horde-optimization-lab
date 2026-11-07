# M0 실험 E0 묶음. Test 패키지로 조건을 연달아 잰다(2026-11-07 사용자가 고른 축소안).
#   full, static: N {250, 500, 1k, 2k, 5k} x P1, P2 x 3회
#   budget, uro, pawn : N {1k, 2k, 5k} x P1 x 3회 (대조군이라 근거리가 보이는 P1과 한계 근처 N만)
# 묶음의 맨 첫 조건만 r0(버리는 실행)을 둔다. 총 88회, 실행 하나 약 85초.
# 실패한 조건은 끝에 목록으로 출력하고, 같은 라벨은 다시 쓰지 않으므로 -Tag를 붙여 run-bench.ps1로 다시 잰다.
param(
    [string]$Tag = ""
)
$ErrorActionPreference = "Stop"
$Bench = "$PSScriptRoot\run-bench.ps1"

$Conditions = @()
foreach ($Variant in @("full", "static")) {
    foreach ($N in @(250, 500, 1000, 2000, 5000)) {
        foreach ($Preset in @("P1", "P2")) { $Conditions += , @($Variant, $N, $Preset) }
    }
}
foreach ($Variant in @("budget", "uro", "pawn")) {
    foreach ($N in @(1000, 2000, 5000)) { $Conditions += , @($Variant, $N, "P1") }
}

$Start = Get-Date
$Failed = @()
$First = $true
foreach ($C in $Conditions) {
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M0", "-Variant", $C[0], "-N", $C[1], "-Preset", $C[2])
    if ($Tag) { $Args += @("-Tag", $Tag) }
    if (-not $First) { $Args += "-SkipR0" }
    $First = $false
    & powershell @Args
    if ($LASTEXITCODE -ne 0) { $Failed += "$($C[0]) N$($C[1]) $($C[2])" }
}
$Elapsed = (Get-Date) - $Start
Write-Host ("E0 DONE: {0} conditions, {1:hh\:mm\:ss}, failed={2}" -f $Conditions.Count, $Elapsed, $Failed.Count)
$Failed | ForEach-Object { Write-Host "FAILED: $_" }
exit $(if ($Failed.Count -gt 0) { 1 } else { 0 })
