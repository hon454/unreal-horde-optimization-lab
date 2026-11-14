# 공용 코드를 고친 뒤의 확인: 단계마다 작은 실행(run-dev.ps1 -Small)을 차례로 돌리고 종료 코드를 모은다(AGENTS.md "완료 확인" 2).
# 사용: powershell -ExecutionPolicy Bypass -File Scripts/run-dev-stages.ps1
param(
    [string[]]$Runs = @("M0:full", "M1:m1mass", "M2:m2mass", "M3:m3mass", "M4:m4offpool", "M5:m5bounds", "M5:q4final")
)
$Fail = 0
foreach ($r in $Runs) {
    $stage, $variant = $r -split ":"
    $out = & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\run-dev.ps1" -Small -Stage $stage -Variant $variant 2>&1 | Out-String
    $code = $LASTEXITCODE
    $summary = ($out -split "`n" | Where-Object { $_ -match "lab_summary" } | Select-Object -Last 1)
    if ($summary) { $summary = $summary.Substring($summary.IndexOf("lab_summary")).Trim() }
    Write-Host "$stage $variant exit=$code $summary"
    if ($code -ne 0) { $Fail++ }
}
Write-Host "FAILED: $Fail"
exit $Fail
