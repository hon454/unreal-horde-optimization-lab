# 다른 스크립트가 불러 쓴다. 프로젝트 경로와 엔진 경로를 정한다.
#
# 엔진 경로는 저장소에 적지 않는다. HordeOptLab.uproject의 EngineAssociation을 이 PC의 레지스트리에서 찾는다.
# 에디터가 .uproject를 열 때 쓰는 것과 같은 방법이라 에디터와 스크립트가 항상 같은 엔진을 쓴다.
# 소스 빌드만 쓴다(AGENTS.md "소스 빌드 엔진만 쓴다"): HKCU\Software\Epic Games\Unreal Engine\Builds 의 <EngineAssociation> 값.
# DS 실험실(ue-dedicated-server-optimization-lab) Scripts/common.ps1에서 가져와 줄였다.

$ProjectDir = (Resolve-Path "$PSScriptRoot\..").Path
$Project = "$ProjectDir\HordeOptLab.uproject"

function Resolve-UERoot {
    $Association = (Get-Content $Project -Raw | ConvertFrom-Json).EngineAssociation

    $Builds = Get-ItemProperty "HKCU:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction SilentlyContinue
    if ($Builds -and $Builds.PSObject.Properties[$Association]) {
        return $Builds.PSObject.Properties[$Association].Value
    }

    throw "Engine '$Association' (EngineAssociation in HordeOptLab.uproject) is not a registered source build on this PC. Right-click HordeOptLab.uproject and choose 'Switch Unreal Engine version'."
}

$UE_ROOT = (Resolve-UERoot).TrimEnd('\', '/')
$Editor = "$UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe"
