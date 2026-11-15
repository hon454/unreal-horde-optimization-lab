# 공용 코드를 고친 뒤의 확인: 사다리 단계마다 작은 실행(run-dev.ps1 -Small)을 차례로 돌리고 종료 코드를 모은다(AGENTS.md "완료 확인" 2).
# 사용: powershell -ExecutionPolicy Bypass -File Scripts/run-dev-stages.ps1
#       빼 보기까지: -Ablations (단계 8에 스위치 하나씩, Docs/Guides/commands.md "빼 보기")
param(
    [string[]]$Stages = @("1", "2", "3", "4", "5", "6", "7", "8"),
    [switch]$Ablations
)
# -File로 부르면 배열이 "1,2,3" 한 문자열로 넘어오므로 쉼표로 다시 나눈다
$Stages = @($Stages | ForEach-Object { $_ -split "," } | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$Runs = @($Stages | ForEach-Object { @{ Tag = "s$_"; Stage = $_; Extra = "" } })
if ($Ablations) {
    $Runs += @(
        @{ Tag = "a8hyd"; Stage = "8"; Extra = "-LabHydrate=0" },
        @{ Tag = "a8bud"; Stage = "8"; Extra = "-LabScreenSigBudget=0" },
        @{ Tag = "a8fix"; Stage = "8"; Extra = "-LabHydrateFixMask=0" },
        @{ Tag = "a8hold"; Stage = "8"; Extra = "-LabRepHold=0" },
        @{ Tag = "a8pool"; Stage = "8"; Extra = "-LabEntityPool=0" },
        @{ Tag = "a8sim"; Stage = "8"; Extra = "-LabSimLod=0" },
        @{ Tag = "a8iskm"; Stage = "8"; Extra = "-LabIskm=0" },
        @{ Tag = "a8bnd"; Stage = "8"; Extra = "-LabFixedBounds=0" },
        @{ Tag = "a8far"; Stage = "8"; Extra = "-LabFarCull=0" },
        @{ Tag = "a8fx"; Stage = "8"; Extra = "-LabCrowdFx=0" },
        @{ Tag = "a8sep"; Stage = "8"; Extra = "-LabSeparate=0" },
        @{ Tag = "a8cap"; Stage = "8"; Extra = "-LabActorCollision=0" },
        @{ Tag = "a8res"; Stage = "8"; Extra = "-LabIskmTrackReserve=0" },
        @{ Tag = "a8fxf"; Stage = "8"; Extra = "-LabFxOrbFollow=0" },
        @{ Tag = "a8stp"; Stage = "8"; Extra = "-LabBoundsStepM=0" },
        @{ Tag = "a8shd"; Stage = "8"; Extra = "-LabFarShadow=1" },
        @{ Tag = "s8still"; Stage = "8"; Extra = "-LabOrbOrbitSpeed=0" }
    )
}
$Fail = 0
foreach ($r in $Runs) {
    $a = @("-ExecutionPolicy", "Bypass", "-File", "$PSScriptRoot\run-dev.ps1", "-Small", "-Stage", $r.Stage, "-Tag", $r.Tag)
    if ($r.Extra) { $a += @("-Extra", $r.Extra) }
    $out = & powershell @a 2>&1 | Out-String
    $code = $LASTEXITCODE
    $summary = ($out -split "`n" | Where-Object { $_ -match "lab_summary" } | Select-Object -Last 1)
    if ($summary) { $summary = $summary.Substring($summary.IndexOf("lab_summary")).Trim() }
    Write-Host "$($r.Tag) exit=$code $summary"
    if ($code -ne 0) { $Fail++ }
}
Write-Host "FAILED: $Fail"
exit $Fail
