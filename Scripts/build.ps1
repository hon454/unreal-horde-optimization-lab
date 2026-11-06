. "$PSScriptRoot\common.ps1"

# 에디터나 실행이 떠 있으면 DLL이 잠겨 빌드가 실패하고, 측정 중이면 빌드가 그 실행의 CPU를 쓴다.
$Running = @(Get-Process UnrealEditor, HordeOptLab -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) {
    Write-Host "FAIL: UnrealEditor or HordeOptLab is running (pid $($Running.Id -join ', ')). Close it, then build."
    exit 1
}

& "$UE_ROOT\Engine\Build\BatchFiles\Build.bat" HordeOptLabEditor Win64 Development "-project=$Project" -waitmutex
exit $LASTEXITCODE
