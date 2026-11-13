# M5 트레이스 4회(Worklog/M5.md "트레이스 분석"): 20k P1, 2종. m4offpool(M4) 병렬, m5iskm(M5) 병렬, m4offpool 직렬, m5iskm 직렬. 라벨 태그 tracepar / traceser.
# 직렬은 -LabFullyParallel=0. 실패하면 라벨을 재사용하지 않고 번호를 올려(r2) 한 번 다시 돈다. 메타데이터 검사는 run-m5-all.ps1과 같다(M5 단계는 Lab/IskmInstances 중앙값 > 0). 요약·커밋은 하지 않는다.
param([switch]$DryRun, [string]$SerialArgs = "-LabFullyParallel=0")
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root
$Bench = "$PSScriptRoot\run-bench.ps1"
$OutCsv = "$Root\Saved\Profiling\CSV"
$StageOf = @{ m4offpool = "M4"; m5iskm = "M5" }
$Plan = @(
    @{ V = "m4offpool"; Tag = "tracepar"; Extra = "" },
    @{ V = "m5iskm"; Tag = "tracepar"; Extra = "" },
    @{ V = "m4offpool"; Tag = "traceser"; Extra = $SerialArgs },
    @{ V = "m5iskm"; Tag = "traceser"; Extra = $SerialArgs }
)
foreach ($R in $Plan) { Write-Host ("M5-{0}-N20000-P1{1}-r1 (retry: r2)  stage={2} extra={3}" -f $R.V, $R.Tag, $StageOf[$R.V], $R.Extra) }
if ($DryRun) { exit 0 }
if (@(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealEditor|UnrealBuildTool|HordeOptLab)" }).Count -gt 0) { Write-Host "FAIL preflight: editor/build/game running"; exit 2 }
$Bad = $false
foreach ($R in $Plan) {
    $Stage = $StageOf[$R.V]
    $Done = $false
    foreach ($Idx in 1, 2) {
        $Label = "M5-$($R.V)-N20000-P1$($R.Tag)-r$Idx"
        if ((Test-Path "$Root\Saved\Traces\$Label.utrace") -or (Test-Path "$OutCsv\$Label.csv")) { Write-Host "SKIP: $Label already used"; continue }
        Write-Host "RUN $Label"
        $Args = @("-ExecutionPolicy", "Bypass", "-File", $Bench, "-Milestone", "M5", "-Variant", $R.V, "-N", 20000, "-Preset", "P1", "-Stage", $Stage, "-RunIndex", $Idx, "-Types", 2, "-Tag", $R.Tag, "-Trace")
        if ($R.Extra) { $Args += @("-ExtraArgs", $R.Extra) }
        & powershell @Args
        if ($LASTEXITCODE -ne 0) { Write-Host "FAILED: $Label (next number if retry is left)"; continue }
        $Text = (Get-Content "$OutCsv\$Label.csv" -Tail 3) -join "`n"
        $Types = [regex]::Match($Text, "\[labtypes\],([^,]*)").Groups[1].Value
        $Lod = [regex]::Match($Text, "\[labsimlod\],([^,]*)").Groups[1].Value
        $Ok = ($Types -eq "2") -and ($Lod -ne "-") -and ($Lod -ne "")
        if ($Ok -and $Stage -eq "M5") {
            $Out = (& py -3 -I "$PSScriptRoot\check-iskm.py" "$OutCsv\$Label.csv" 2>&1 | Out-String).Trim()
            if ($LASTEXITCODE -ne 0) { $Ok = $false; Write-Host "IskmInstances median=$Out" }
        }
        Write-Host "CHECK $Label labtypes=$Types labsimlod=$Lod ok=$Ok"
        if ($Ok) { $Done = $true; break }
    }
    if (-not $Done) { Write-Host "STOP: $($R.V) $($R.Tag) failed"; $Bad = $true; break }
}
exit $(if ($Bad) { 1 } else { 0 })
