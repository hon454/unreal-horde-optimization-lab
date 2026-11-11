# Q4 품질 올리기 묶음(Docs/Worklog/Q4.md). run-bench.ps1을 조건마다 차례로 부른다. run-bench.ps1은 고치지 않는다.
# 구성은 q2crowd(= m5bounds + -LabCrowdFx=1)에 액터 상한(-LabActorCap)과 화면 안 전환 거리(-LabHydrateM)를 얹는다. run-bench.ps1의 ValidateSet에 q2crowd가 없어
# 라벨의 변형은 m5bounds이고 배경 무리는 메타데이터 LabCrowdFx=1로 확인한다. 단계는 M5, 2종.
# -Jobs: "N:프리셋:상한:거리:횟수:태그앞:태그뒤"를 ;로 잇는다. 태그 = <태그앞>c<상한>h<거리><태그뒤>(예: gc750h40, c500h30aa). 라벨 Q4-m5bounds-N<N>-<프리셋><태그>-r<i>.
# -FirstWithR0: 첫 조건만 r0(묶음 첫 실행, 드라이버 PSO 캐시용, summary에서 빠진다)부터 잰다. 나머지는 r1부터.
# 실패한 실행은 run-bench.ps1이 FAIL로 알리고 이 스크립트는 다음 조건으로 넘어간다. 끝에 실패 수를 출력한다.
param(
    [Parameter(Mandatory)][string]$Jobs,
    [switch]$FirstWithR0
)
$Failed = @()
$First = $true
foreach ($Job in $Jobs.Split(";", [StringSplitOptions]::RemoveEmptyEntries)) {
    $f = $Job.Trim().Split(":")
    $N = [int]$f[0]; $Preset = $f[1]; $Cap = [int]$f[2]; $Hyd = [int]$f[3]; $Runs = [int]$f[4]
    $Tag = "$($f[5])c$($Cap)h$($Hyd)$($f[6])"
    $Extra = "-LabCrowdFx=1 -LabActorCap=$Cap -LabHydrateM=$Hyd"
    $BenchArgs = @{ Milestone = "Q4"; Stage = "M5"; Variant = "m5bounds"; Types = 2; N = $N; Preset = $Preset; Runs = $Runs; Tag = $Tag; ExtraArgs = $Extra }
    if (-not ($First -and $FirstWithR0)) { $BenchArgs.SkipR0 = $true }
    $First = $false
    Write-Host "JOB $Job -> tag=$Tag $(Get-Date -Format 'HH:mm:ss')"
    & "$PSScriptRoot\run-bench.ps1" @BenchArgs
    if ($LASTEXITCODE -ne 0) { $Failed += $Tag + "-N$N-$Preset" }
}
Write-Host "Q4 BATCH DONE $(Get-Date -Format 'HH:mm:ss') failed=$($Failed.Count) $($Failed -join ', ')"
