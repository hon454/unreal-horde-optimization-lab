# 깨끗한 클론에서 git 밖 에셋을 한 번에 복구한다. 가이드는 Docs/Guides/setup.md.
#
# 전제: Fab의 Paragon: Minions와 Niagara Examples Pack을 런처 엔진의 빈 프로젝트(에셋 호스트)에 "프로젝트에 추가"로 받아 둔다.
#   팩은 호스트의 Content/ParagonMinions/, Content/NiagaraExamples/에 놓인다. 런처는 소스 빌드를 엔진으로 인식하지 못해
#   HordeOptLab에 팩을 직접 추가할 수 없다(Docs/Reference/references.md "참고 프로젝트").
#
# 하는 일(순서는 각 스크립트 헤더가 정한 것):
#   1. migrate-minion/siege/orbfx  호스트 엔진 커맨드렛 + Scripts/migrate-assets.py. 쓰는 에셋과 의존성만 Content/로 옮긴다
#                                  (대상에 이미 있는 패키지는 건너뛴다, 덮어쓰지 않는다)
#   2. static-small/large          Scripts/make-static-minion.py   -> Content/Lab/Derived/
#   3. vat-small/large             Scripts/bake-vat.py             -> Content/Lab/VAT/Baked/
#      vat-lod4-small/large        Scripts/bake-vat.py LAB_VAT_LOD=4 (입자 전용 저폴리 메시, *_Bone_Lod4)
#   4. iskm, iskm-verify           Scripts/create-iskm.py          -> Content/Lab/ISKM/Baked/
#   5. Scripts/restore-assets.manifest.txt와 실제 파일을 대조해 빠진 것을 보고한다.
#   2~4는 프로젝트 엔진(Scripts/common.ps1이 찾는 소스 빌드)의 커맨드렛이다. AnimBP·Niagara 시스템·맵·바닥 머티리얼은 커밋돼 있어 만들지 않는다.
#
# 인자:
#   -AssetHost <.uproject>   에셋 호스트 프로젝트. 기본은 형제 폴더 ..\unreal-horde-references\AssetHost\AssetHost.uproject
#   -HostEngineRoot <폴더>   호스트를 여는 엔진 폴더. 기본은 호스트 .uproject의 EngineAssociation으로 찾는다
#                            (버전 문자열 "5.8"이면 런처 설치 목록, GUID면 HKCU 소스 빌드 등록)
#   -Force                   산출물이 있어도 다시 만든다. Lab/Derived, Lab/VAT/Baked, Lab/ISKM/Baked를 Saved/RestoreBackup/<시각>/로 옮긴 뒤 만든다(지우지 않는다).
#                            팩 이전은 다시 돌려도 이미 있는 패키지를 덮어쓰지 않는다
#   -DryRun                  검사 결과와 부를 명령만 출력한다. 아무것도 실행하거나 옮기지 않는다
# 로그: Saved/Logs/Restore/<단계>.log. 종료 코드: 0 성공, 1 검사 실패 또는 단계 실패 또는 필수 산출물 누락.
param(
    [string]$AssetHost = "",
    [string]$HostEngineRoot = "",
    [switch]$Force,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

# 프로젝트 엔진. common.ps1은 등록된 소스 빌드가 없으면 throw한다. 건식 실행은 그 경우에도 명령을 보여 준다.
$EngineError = $null
try { . "$PSScriptRoot\common.ps1" } catch { $EngineError = $_.Exception.Message }
if (-not $ProjectDir) { $ProjectDir = (Resolve-Path "$PSScriptRoot\..").Path; $Project = "$ProjectDir\HordeOptLab.uproject" }
$Content = "$ProjectDir\Content"
$LogDir = "$ProjectDir\Saved\Logs\Restore"
$Problems = New-Object System.Collections.Generic.List[string]

function Say($s) { Write-Host $s }
function Problem($s) { $Problems.Add($s); Write-Host "MISSING: $s" -ForegroundColor Yellow }

# ---- 1. 검사 ----
Say "== Check"
if ($EngineError) {
    Problem "Project engine: $EngineError Register the UE 5.8.4 source build (run Setup.bat and GenerateProjectFiles.bat in it, then right-click HordeOptLab.uproject > Switch Unreal Engine version). See Docs/Guides/setup.md."
    $ProjectCmd = "<UE 5.8.4 source build>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
} else {
    $ProjectCmd = "$UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    if (Test-Path $ProjectCmd) { Say "OK  project engine: $UE_ROOT" }
    else { Problem "Project engine has no $ProjectCmd. Build the engine (UnrealEditor Win64 Development) first." }
}

# 이전 시드(migrate-assets.py SETS와 같다). 호스트에 있어야 하고, 이전 뒤 프로젝트에 있어야 한다.
$MinionRoot = "ParagonMinions\Characters\Minions\Down_Minions"
$Seeds = [ordered]@{
    minion = @("$MinionRoot\Meshes\Minion_Lane_Melee_Dawn", "$MinionRoot\Meshes\Minion_Lane_Melee_Physics",
               "$MinionRoot\Animations\Melee\NonCombat_JogFwd", "$MinionRoot\Animations\Melee\HitReact_Front", "$MinionRoot\Animations\Melee\Death_A")
    siege  = @("$MinionRoot\Meshes\Minion_Lane_Siege_Dawn", "$MinionRoot\Meshes\Minion_Lane_Siege_Physics",
               "$MinionRoot\Animations\Siege\Jog_Fwd_NonCombat", "$MinionRoot\Animations\Siege\HitReact_Front", "$MinionRoot\Animations\Siege\Death_A")
    orbfx  = @("NiagaraExamples\FX_Ribbons\NS_TeslaCoil", "NiagaraExamples\FX_Misc\NS_Boundary_Cylinder", "NiagaraExamples\Gallery\StaticMesh\SM_TorchBase",
               "NiagaraExamples\Gallery\Materials\M_Target_Metal", "NiagaraExamples\Materials\MI_Mesh_Overlay_TeslaCoil")
}
$PackOf = @{ minion = "Paragon: Minions (ParagonMinions)"; siege = "Paragon: Minions (ParagonMinions)"; orbfx = "Niagara Examples Pack (NiagaraExamples)" }

# 팩 이전이 필요할 때만 호스트를 검사한다(이미 옮겨 둔 PC에서는 호스트가 없어도 된다).
$NeedMigrate = [bool]$Force
foreach ($Set in $Seeds.Keys) { foreach ($P in $Seeds[$Set]) { if (-not (Test-Path "$Content\$P.uasset")) { $NeedMigrate = $true } } }
$HostProblems = New-Object System.Collections.Generic.List[string]
function HostProblem($s) {
    if ($NeedMigrate) { Problem $s } else { $HostProblems.Add($s) }
}
if (-not $NeedMigrate) { Say "OK  migrated pack assets are already in Content/ (asset host not needed unless -Force)" }

if (-not $AssetHost) { $AssetHost = Join-Path (Split-Path $ProjectDir -Parent) "unreal-horde-references\AssetHost\AssetHost.uproject" }
$HostCmd = "<host engine>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$HostContent = $null
if (-not (Test-Path $AssetHost)) {
    HostProblem "Asset host project not found: $AssetHost. Create a blank Blueprint project with the launcher UE 5.8, add both Fab packs to it, and pass -AssetHost <that .uproject>. See Docs/Guides/setup.md."
} else {
    $AssetHost = (Resolve-Path $AssetHost).Path
    $HostContent = Join-Path (Split-Path $AssetHost -Parent) "Content"
    Say "OK  asset host: $AssetHost"

    if (-not $HostEngineRoot) {
        $Assoc = (Get-Content $AssetHost -Raw | ConvertFrom-Json).EngineAssociation
        if ($Assoc -match '^\{') {
            $Builds = Get-ItemProperty "HKCU:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction SilentlyContinue
            if ($Builds -and $Builds.PSObject.Properties[$Assoc]) { $HostEngineRoot = $Builds.PSObject.Properties[$Assoc].Value }
        } elseif ($Assoc) {
            $Dat = "$env:ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat"
            if (Test-Path $Dat) {
                $Hit = (Get-Content $Dat -Raw | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -eq "UE_$Assoc" } | Select-Object -First 1
                if ($Hit) { $HostEngineRoot = $Hit.InstallLocation }
            }
        }
        if (-not $HostEngineRoot) { HostProblem "Cannot find the engine for the asset host (EngineAssociation '$Assoc'). Pass -HostEngineRoot <launcher engine folder, e.g. C:\Program Files\Epic Games\UE_5.8>." }
    }
    if ($HostEngineRoot) {
        $HostEngineRoot = $HostEngineRoot.TrimEnd('\', '/')
        $HostCmd = "$HostEngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
        if (Test-Path $HostCmd) { Say "OK  host engine: $HostEngineRoot" } else { HostProblem "Host engine has no $HostCmd." }
    }
    # C++ 모듈이 있는 호스트는 다른 엔진으로 컴파일되면 커맨드렛이 로그 없이 종료 코드 1로 끝난다(troubleshooting.md "AssetHost").
    if (Test-Path (Join-Path (Split-Path $AssetHost -Parent) "Source")) {
        Say "NOTE asset host has a Source/ folder. If a migrate step exits with code 1 and no Python output, see Docs/Guides/troubleshooting.md (AssetHost)."
    }
}

if ($HostContent -and (Test-Path $HostContent)) {
    foreach ($Set in $Seeds.Keys) {
        $Missing = @($Seeds[$Set] | Where-Object { -not (Test-Path "$HostContent\$_.uasset") })
        if ($Missing.Count -eq 0) { Say "OK  host pack for '$Set': $($PackOf[$Set])" }
        else { HostProblem "Asset host lacks $($PackOf[$Set]): $HostContent\$($Missing[0]).uasset. Add the pack to the asset host from Fab (Add to project). Links: Docs/Guides/setup.md." }
    }
}

foreach ($H in $HostProblems) { Say "NOTE (not needed now) $H" }

if (-not $DryRun) {
    $Running = @(Get-Process UnrealEditor, UnrealEditor-Cmd, HordeOptLab -ErrorAction SilentlyContinue)
    if ($Running.Count -gt 0) { Problem "UnrealEditor or HordeOptLab is running (pid $($Running.Id -join ', ')). Close it first; commandlets write into Content/." }
}

if ($Problems.Count -gt 0 -and -not $DryRun) {
    Say "FAIL: $($Problems.Count) check(s) failed. Nothing was changed."
    exit 1
}

# ---- 2. 단계 ----
# 산출물(Content/ 기준, 확장자 .uasset 생략). 있으면 그 단계를 건너뛴다(-Force가 아니면).
function VatOutputs($Token) {
    $List = @("M_${Token}Actor")
    foreach ($Mode in "Bone", "Vertex") {
        $List += "DA_${Token}VAT_$Mode", "M_${Token}VAT_$Mode", "MI_${Token}VAT_$Mode", "SM_${Token}VAT_$Mode"
    }
    $List += "T_${Token}VAT_Bone_BonePosition", "T_${Token}VAT_Bone_BoneRotation", "T_${Token}VAT_Bone_BoneWeight",
             "T_${Token}VAT_Vertex_VertexNormal", "T_${Token}VAT_Vertex_VertexPosition"
    return $List | ForEach-Object { "Lab\VAT\Baked\$_" }
}
# 입자 전용 저폴리 굽기(bake-vat.py LAB_VAT_LOD=N): Bone 모드 한 벌, 이름 끝 _Lod<N>. NS_LabCrowd와 MI_LabCrowd*가 LOD4를 쓴다(create-crowd-fx.md 6절).
function VatLodOutputs($Token, $Lod) {
    $B = "${Token}VAT_Bone_Lod$Lod"
    return @("SM_$B", "DA_$B", "MI_$B", "T_${B}_BonePosition", "T_${B}_BoneRotation", "T_${B}_BoneWeight") | ForEach-Object { "Lab\VAT\Baked\$_" }
}
$IskmOutputs = foreach ($T in "LabMinion", "LabMinionLarge") { "Lab\ISKM\Baked\ASTP_$T", "Lab\ISKM\Baked\M_${T}ISKM", "Lab\ISKM\Baked\SK_${T}ISKM" }

# -script 경로는 슬래시로 넘긴다. 역슬래시면 "Games\UE_5.8.4"의 "\UE"가 사라져 파일을 못 찾는다
# (2026-11-12 검증 실행 로그: Could not load Python file 'G:/Epic Games_5.8.4/...').
$ScriptDir = $PSScriptRoot -replace '\\', '/'
$Steps = @()
foreach ($Set in $Seeds.Keys) {
    $Steps += @{ Name = "migrate-$Set"; Exe = $HostCmd; ProjectFile = $AssetHost; Script = "migrate-assets.py"
                 Env = @{ LAB_MIGRATE_SET = $Set; LAB_MIGRATE_DRY = "0"; LAB_MIGRATE_DEST = $Content }
                 Flags = @("-nullrhi"); Outputs = $Seeds[$Set] }
}
# 프로젝트 엔진으로 도는 굽기·파생 단계는 이 표 한 곳에 모은다. 순서대로 돈다(앞 단계 산출물을 뒤 단계가 읽는다).
# 칸: 이름, 스크립트, 환경 변수, 산출물(있으면 건너뜀), [로그 성공 표시], [앞 단계 이름: 그 단계가 돌았을 때만 돈다].
# 새 굽기(예: 입자 전용 저폴리 VAT)는 해당 줄 뒤에 한 줄을 더하고, 산출물을 Scripts/restore-assets.manifest.txt에도 더한다.
$BakeSteps = @(
    @{ Name = "static-small"; Script = "make-static-minion.py"; Env = @{ LAB_UNIT = "small" }; Outputs = @("Lab\Derived\SM_LabMinion") }
    @{ Name = "static-large"; Script = "make-static-minion.py"; Env = @{ LAB_UNIT = "large" }; Outputs = @("Lab\Derived\SM_LabMinionLarge", "Lab\Derived\M_LabMinionLargeStatic") }
    @{ Name = "vat-small";    Script = "bake-vat.py";           Env = @{ LAB_UNIT = "small" }; Outputs = (VatOutputs "LabMinion") }
    @{ Name = "vat-large";    Script = "bake-vat.py";           Env = @{ LAB_UNIT = "large" }; Outputs = (VatOutputs "LabMinionLarge") }
    # LOD0 굽기의 M_<TOKEN>VAT_Bone을 부모로 쓰므로 vat-small·vat-large 뒤에 돈다.
    @{ Name = "vat-lod4-small"; Script = "bake-vat.py";         Env = @{ LAB_UNIT = "small"; LAB_VAT_LOD = "4" }; Outputs = (VatLodOutputs "LabMinion" 4) }
    @{ Name = "vat-lod4-large"; Script = "bake-vat.py";         Env = @{ LAB_UNIT = "large"; LAB_VAT_LOD = "4" }; Outputs = (VatLodOutputs "LabMinionLarge" 4) }
    # create-iskm.py는 종류별 실패를 잡아 로그 "LABISKM result failed=none"으로만 알린다. 그 줄을 성공 표시로 본다.
    @{ Name = "iskm";         Script = "create-iskm.py";        Env = @{ LAB_UNIT = "" }; Outputs = $IskmOutputs; LogMarker = "LABISKM result failed=none" }
    @{ Name = "iskm-verify";  Script = "create-iskm.py";        Env = @{ LAB_UNIT = ""; LAB_ISKM_VERIFY = "1" }; Outputs = @(); LogMarker = "LABISKM result failed=none"; After = "iskm" }
)
$Render = @("-AllowCommandletRendering")  # 스켈레탈 메시 렌더 데이터를 읽는다(-nullrhi면 실패, 각 스크립트 헤더)
foreach ($B in $BakeSteps) {
    $B.Exe = $ProjectCmd; $B.ProjectFile = $Project; $B.Flags = $Render
    $Steps += $B
}

function Has-All($List) { foreach ($P in $List) { if (-not (Test-Path "$Content\$P.uasset")) { return $false } }; return $true }

function Command-Line($S, $Log) {
    $EnvText = ($S.Env.GetEnumerator() | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join " "
    $ArgText = "`"$($S.ProjectFile)`" -run=pythonscript -script=`"$ScriptDir/$($S.Script)`" -unattended -nosplash $($S.Flags -join ' ') -abslog=`"$Log`""
    return "[$EnvText] `"$($S.Exe)`" $ArgText"
}

if ($Force) {
    $Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    foreach ($Dir in "Lab\Derived", "Lab\VAT\Baked", "Lab\ISKM\Baked") {
        if (Test-Path "$Content\$Dir") {
            $Dest = "$ProjectDir\Saved\RestoreBackup\$Stamp\$Dir"
            if ($DryRun) { Say "WOULD MOVE Content\$Dir -> $Dest"; continue }
            New-Item -ItemType Directory -Force -Path (Split-Path $Dest -Parent) | Out-Null
            Move-Item "$Content\$Dir" $Dest
            Say "Moved Content\$Dir -> $Dest"
        }
    }
}

Say ""
Say "== Steps$(if ($DryRun) { ' (dry run: nothing is executed)' })"
$Ran = @{}
if (-not $DryRun) { New-Item -ItemType Directory -Force -Path $LogDir | Out-Null }
foreach ($S in $Steps) {
    $Log = "$LogDir\$($S.Name).log"
    $Skip = $false
    if ($S.After) { $Skip = -not $Ran[$S.After] -and -not $Force }
    elseif (-not $Force -and $S.Outputs.Count -gt 0 -and (Has-All $S.Outputs)) { $Skip = $true }

    if ($Skip) { Say "SKIP $($S.Name): $(if ($S.After) { "$($S.After) did not run" } else { 'outputs already exist (use -Force to rebuild)' })"; continue }
    Say "RUN  $($S.Name)"
    Say "     $(Command-Line $S $Log)"
    if ($DryRun) { $Ran[$S.Name] = $true; continue }

    $Saved = @{}
    foreach ($K in $S.Env.Keys) { $Saved[$K] = [Environment]::GetEnvironmentVariable($K); [Environment]::SetEnvironmentVariable($K, $(if ($S.Env[$K]) { $S.Env[$K] } else { $null })) }
    $Started = Get-Date
    try {
        $ArgList = @("`"$($S.ProjectFile)`"", "-run=pythonscript", "-script=`"$ScriptDir/$($S.Script)`"", "-unattended", "-nosplash") + $S.Flags + @("-abslog=`"$Log`"")
        $P = Start-Process -FilePath $S.Exe -ArgumentList $ArgList -NoNewWindow -Wait -PassThru -RedirectStandardOutput "$Log.stdout.txt" -RedirectStandardError "$Log.stderr.txt"
        $Code = $P.ExitCode
    } finally {
        foreach ($K in $Saved.Keys) { [Environment]::SetEnvironmentVariable($K, $Saved[$K]) }
    }
    $Secs = [int]((Get-Date) - $Started).TotalSeconds
    $MissingOut = @($S.Outputs | Where-Object { -not (Test-Path "$Content\$_.uasset") })
    $MarkerOk = (-not $S.LogMarker) -or ((Test-Path $Log) -and (Select-String -Path $Log -SimpleMatch $S.LogMarker -Quiet))
    if ($Code -ne 0 -or $MissingOut.Count -gt 0 -or -not $MarkerOk) {
        Say "FAIL $($S.Name): exit code $Code, ${Secs}s, missing outputs $($MissingOut.Count)$(if (-not $MarkerOk) { ", no '$($S.LogMarker)' in log" })"
        if ($MissingOut.Count -gt 0) { Say "     first missing: Content\$($MissingOut[0]).uasset" }
        Say "     log: $Log"
        if (Test-Path $Log) { Get-Content $Log | Select-String -Pattern "LAB[A-Z]* |Error:" | Select-Object -Last 15 | ForEach-Object { Say "     | $($_.Line)" } }
        Say "     See Docs/Guides/troubleshooting.md. Rerun this script after fixing; finished steps are skipped."
        exit 1
    }
    Say "OK   $($S.Name) (${Secs}s)"
    $Ran[$S.Name] = $true
}

if ($DryRun) {
    Say ""
    if ($Problems.Count -gt 0) { Say "DRY RUN: $($Problems.Count) check(s) would stop a real run (see MISSING above)." }
    else { Say "DRY RUN: checks passed." }
    exit 0
}

# ---- 3. 대조 ----
Say ""
Say "== Verify against Scripts\restore-assets.manifest.txt"
$Required = @($Steps | ForEach-Object { $_.Outputs }) | Sort-Object -Unique
$Lines = Get-Content "$PSScriptRoot\restore-assets.manifest.txt" -Encoding UTF8 | Where-Object { $_ -match '^\[' }
$MissReq = New-Object System.Collections.Generic.List[string]
$MissOther = New-Object System.Collections.Generic.List[string]
$Count = 0
foreach ($L in $Lines) {
    $Rel = ($L -replace '^\[[a-z0-9-]+\]\s+', '') -replace '/', '\'
    $Count++
    if (-not (Test-Path -LiteralPath "$Content\$Rel")) {
        if ($Required -contains ($Rel -replace '\.uasset$', '')) { $MissReq.Add($Rel) } else { $MissOther.Add($Rel) }
    }
}
foreach ($R in $Required) { if (-not (Test-Path "$Content\$R.uasset") -and -not $MissReq.Contains("$R.uasset")) { $MissReq.Add("$R.uasset") } }
Say "Manifest: $Count files, missing $($MissReq.Count + $MissOther.Count) (required $($MissReq.Count))"
foreach ($M in $MissReq) { Say "  MISSING (required) Content\$M" }
foreach ($M in $MissOther) { Say "  missing (dependency) Content\$M" }
if ($MissOther.Count -gt 0) { Say "  Dependency files follow the pack version; a different pack version can move or rename them. The game needs only the required files." }
if ($MissReq.Count -gt 0) { Say "FAIL: required assets are missing."; exit 1 }
Say "DONE: assets restored. Next: Scripts/build.ps1, then Scripts/run-dev.ps1 -Small (Docs/Guides/setup.md)."
exit 0
