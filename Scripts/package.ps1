# Test(기본) 구성 Win64 패키지를 만든다. 수치는 이 패키지에서만 잰다(ADR-0001).
# 결과: Packaged/<Config>/Windows/HordeOptLab.exe. 패키지 폴더에 커밋 해시(commit.txt)를 남긴다(commands.md "측정 절차" 2번).
param(
    [ValidateSet("Test", "Development", "Shipping")][string]$Config = "Test"
)
. "$PSScriptRoot\common.ps1"

$Running = @(Get-Process UnrealEditor, HordeOptLab, HordeOptLab-Win64-Test -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) {
    Write-Host "FAIL: an editor or game is running (pid $($Running.Id -join ', ')). Close it, then package."
    exit 1
}

$Out = "$ProjectDir\Packaged\$Config"
$Start = Get-Date
& "$UE_ROOT\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
    "-project=$Project" -platform=Win64 "-clientconfig=$Config" `
    -build -cook -stage -pak -archive "-archivedirectory=$Out" `
    -map=/Game/Lab/Maps/L_Bench -noP4 -utf8output -unattended -nodebuginfo
$Code = $LASTEXITCODE
$Elapsed = (Get-Date) - $Start
if ($Code -eq 0) {
    $Commit = (git -C $ProjectDir rev-parse HEAD).Trim()
    $Dirty = if (git -C $ProjectDir status --porcelain) { " (dirty)" } else { "" }
    Set-Content -Path "$Out\commit.txt" -Value "$Commit$Dirty" -Encoding UTF8
    Write-Host "PACKAGED: $Out commit=$Commit$Dirty"
}
Write-Host ("ELAPSED: {0:hh\:mm\:ss}" -f $Elapsed)
exit $Code
