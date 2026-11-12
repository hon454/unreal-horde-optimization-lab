# Development 전환 점수 묶음(ADR-0012 결정 4). 최종 구성과 ISKM 끔, 전환 대책 끔, 둘 다 끔을 시드마다 한 번씩 찍고 점수를 낸다.
# Development 에디터 -game 실행이라 값은 참고다(프레임 수치가 아니라 화면 밝기 변화). 절차는 Docs/Worklog/Q3.md "전환 점수".
#   결과: Measurements/<Batch>/transition-dev/runs.csv, 로그 사본: Saved/Logs/<Batch>tr-<조건>-s<시드>.log
#   캡처 600장은 점수를 낸 뒤 지운다(-KeepFrames면 Saved/Screenshots/LabBurst-<Batch>tr-<조건>-s<시드>로 옮긴다).
param(
    [string]$Batch = "Full1",
    [int[]]$Seeds = @(20261106, 20261107, 20261108),
    [string]$Variant = "q4final",
    [switch]$KeepFrames,
    [switch]$DryRun
)
. "$PSScriptRoot\common.ps1"

$Conds = @(
    @{ Name = "iskm-fix";  Sw = "" },
    @{ Name = "vat-fix";   Sw = "-LabIskm=0" },
    @{ Name = "iskm-nofix"; Sw = "-LabHydrateFixMask=0" },
    @{ Name = "vat-nofix";  Sw = "-LabIskm=0 -LabHydrateFixMask=0" }
)
$Burst = "$ProjectDir\Saved\Screenshots\LabBurst"
$OutDir = "$ProjectDir\Measurements\$Batch\transition-dev"
$Csv = "$OutDir\runs.csv"
if (-not $DryRun) {
    New-Item -ItemType Directory -Force $OutDir | Out-Null
    if (-not (Test-Path $Csv)) { "batch,cond,switches,seed,exit,respawns,kills,snipes,events,mean_excess,median_excess,noticeable" | Set-Content -Encoding utf8 $Csv }
}

$Fail = 0
foreach ($Seed in $Seeds) {
    foreach ($C in $Conds) {
        $Tag = "${Batch}tr-$($C.Name)-s$Seed"
        $Extra = ("-LabShotBurst=600 -benchmark -fps=60 " + $C.Sw).Trim()
        Write-Host "=== $Tag : $Extra"
        if ($DryRun) { continue }
        if (Test-Path $Burst) { Get-ChildItem $Burst -File | Remove-Item -Force }
        $Out = & "$PSScriptRoot\run-dev.ps1" -N 10000 -Preset P1 -Seed $Seed -Duration 21 -ShotAt 10 -Stage M5 -Variant $Variant -Extra $Extra 6>&1 | Out-String
        $Code = $LASTEXITCODE
        $Log = "$ProjectDir\Saved\Logs\LabDev-$Variant-N10000-P1.log"
        $LogCopy = "$ProjectDir\Saved\Logs\$Tag.log"
        Copy-Item $Log $LogCopy -Force
        $Frames = @(Get-ChildItem $Burst -File -Filter "f*.png").Count
        $R = ""; $K = ""; $S = ""
        if ($Out -match "respawns=(\d+)") { $R = $Matches[1] }
        if ($Out -match "kills=(\d+)") { $K = $Matches[1] }
        if ($Out -match "snipes=(\d+)") { $S = $Matches[1] }
        $Ev = ""; $Mean = ""; $Med = ""; $Not = ""
        if ($Code -eq 0 -and $Frames -eq 600) {
            $Score = (& py -3 "$PSScriptRoot\transition-score.py" $Burst $LogCopy | Out-String).Trim()
            Write-Host "    $Score"
            if ($Score -match "events (\d+)\s+mean_excess ([\d.]+)\s+median_excess ([\d.]+)\s+noticeable (\d+)/") {
                $Ev = $Matches[1]; $Mean = $Matches[2]; $Med = $Matches[3]; $Not = $Matches[4]
            } else { $Fail++ }
        } else {
            Write-Host "    FAILED exit=$Code frames=$Frames"
            $Fail++
        }
        "$Batch,$($C.Name),$($C.Sw),$Seed,$Code,$R,$K,$S,$Ev,$Mean,$Med,$Not" | Add-Content -Encoding utf8 $Csv
        if ($KeepFrames) {
            Rename-Item $Burst "LabBurst-$Tag"
        } else {
            Get-ChildItem $Burst -File | Remove-Item -Force
        }
    }
}
Write-Host "DONE fail=$Fail"
exit $Fail
