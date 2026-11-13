# M4 추가 직렬 트레이스 3회(Worklog/M4.md "트레이스 분석" 둘째 절): P2 m3mass, P2 m4mass, P1 m3mass. 20k, 2종, -LabFullyParallel=0, 라벨 태그 traceser.
# run-m4-all.ps1과 같은 포그라운드·메타데이터 검사, 실패하면 한 번 재시도. 트레이스 실행의 수치는 비교에 쓰지 않는다. 요약·커밋은 하지 않는다.
param([switch]$DryRun)
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRoot\run-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$Plan = @(
    @{ V = "m3mass"; S = "M3"; P = "P2" },
    @{ V = "m4mass"; S = "M4"; P = "P2" },
    @{ V = "m3mass"; S = "M3"; P = "P1" }
)
foreach ($R in $Plan) { Write-Host ("M4-{0}-N20000-{1}traceser-r1  stage={2}" -f $R.V, $R.P, $R.S) }
if ($DryRun) { exit 0 }
if (@(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" }).Count -gt 0) { Write-Host "FAIL preflight: editor/build/game running"; exit 2 }
$Bad = $false
foreach ($R in $Plan) {
    $Label = "M4-$($R.V)-N20000-$($R.P)traceser-r1"
    $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M4", "-Variant", $R.V, "-N", 20000, "-Preset", $R.P,
              "-Stage", $R.S, "-RunIndex", 1, "-Types", 2, "-Tag", "traceser", "-Trace", "-ExtraArgs", "-LabFullyParallel=0")
    Write-Host "RUN $Label"
    & powershell @Args
    if ($LASTEXITCODE -ne 0) { Write-Host "RETRY once: $Label"; & powershell @Args }
    if ($LASTEXITCODE -ne 0) { Write-Host "STOP: $Label failed"; $Bad = $true; break }
    $Text = (Get-Content "$OutCsv\$Label.csv" -Tail 3) -join "`n"
    $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
    $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value
    $Ok = ($Types -eq "2") -and (($R.S -eq "M4" -and $Lod -ne "-" -and $Lod -ne "") -or ($R.S -eq "M3" -and $Lod -eq "-"))
    Write-Host "CHECK $Label labtypes=$Types labsimlod=$Lod ok=$Ok"
    if (-not $Ok) { $Bad = $true; break }
}
exit $(if ($Bad) { 1 } else { 0 })
