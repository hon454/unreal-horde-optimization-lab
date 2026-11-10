# (-Tag로 라벨 태그를 바꿀 수 있다. 수정 검증 트레이스는 -Tag traceparfix) M4 병렬 트레이스 2회(Worklog/M4.md "병렬 트레이스 2회(P2)"): P2 m3mass, P2 m4mass. 20k, 2종, 병렬(직렬 인자 없음), 라벨 태그 tracepar.
# 실패(포그라운드 FAIL 등)하면 라벨을 재사용하지 않고 번호를 올려(r2) 한 번 다시 돈다. 메타데이터 검사는 run-m4-all.ps1과 같다. 요약·커밋은 하지 않는다.
param([switch]$DryRun, [string]$Tag = "tracepar")
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRoot\run-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$Plan = @(@{ V = "m3mass"; S = "M3"; P = "P2" }, @{ V = "m4mass"; S = "M4"; P = "P2" })
foreach ($R in $Plan) { Write-Host ("M4-{0}-N20000-{1}{3}-r1 (retry: r2)  stage={2}" -f $R.V, $R.P, $R.S, $Tag) }
if ($DryRun) { exit 0 }
if (@(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" }).Count -gt 0) { Write-Host "FAIL preflight: editor/build/game running"; exit 2 }
$Bad = $false
foreach ($R in $Plan) {
    $Done = $false
    foreach ($Idx in 1, 2) {
        $Label = "M4-$($R.V)-N20000-$($R.P)$Tag-r$Idx"
        if ((Test-Path "$Root\Saved\Traces\$Label.utrace") -or (Test-Path "$OutCsv\$Label.csv")) { Write-Host "SKIP: $Label already used"; continue }
        Write-Host "RUN $Label"
        & powershell -ExecutionPolicy Bypass -File $Bench -Milestone M4 -Variant $R.V -N 20000 -Preset $R.P -Stage $R.S -RunIndex $Idx -Types 2 -Tag $Tag -Trace
        if ($LASTEXITCODE -ne 0) { Write-Host "FAILED: $Label (next number if retry is left)"; continue }
        $Text = (Get-Content "$OutCsv\$Label.csv" -Tail 3) -join "`n"
        $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
        $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value
        $Ok = ($Types -eq "2") -and (($R.S -eq "M4" -and $Lod -ne "-" -and $Lod -ne "") -or ($R.S -eq "M3" -and $Lod -eq "-"))
        Write-Host "CHECK $Label labtypes=$Types labsimlod=$Lod ok=$Ok"
        if ($Ok) { $Done = $true; break }
    }
    if (-not $Done) { Write-Host "STOP: $($R.V) failed"; $Bad = $true; break }
}
exit $(if ($Bad) { 1 } else { 0 })
