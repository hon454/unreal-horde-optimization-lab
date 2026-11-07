# Fab 원본 팩(AssetHost)에서 쓰는 에셋과 그 의존성만 HordeOptLab/Content로 옮긴다.
# 경로는 /Game/<팩>/... 그대로 유지되고, 그 폴더들은 gitignore 대상이다(Fab 재배포 금지).
# 엔진 API: IAssetTools::MigratePackages(PackageNames, DestinationPath, FMigrationOptions)
#   (Engine/Source/Developer/AssetTools/Public/IAssetTools.h:633, BlueprintCallable)
#
# AssetHost 프로젝트를 연 엔진의 커맨드렛으로 실행한다. 명령은 Docs/Guides/commands.md "에셋 마이그레이션".
#   LAB_MIGRATE_SET   minion | orbfx (기본 minion)
#   LAB_MIGRATE_DEST  옮길 대상 Content 폴더(필수)
#   LAB_MIGRATE_DRY   1이면 의존성 목록과 크기만 출력(기본 1)
import os
import unreal

ROOT = "/Game/ParagonMinions/Characters/Minions/Down_Minions"
SETS = {
    # M0 근접 미니언
    "minion": [
        ROOT + "/Meshes/Minion_Lane_Melee_Dawn",
        ROOT + "/Meshes/Minion_Lane_Melee_Physics",
        ROOT + "/Animations/Melee/NonCombat_JogFwd",  # 걷기
        ROOT + "/Animations/Melee/HitReact_Front",    # 피격
        ROOT + "/Animations/Melee/Death_A",           # 죽기
    ],
    # 오브 자리에 상시로 띄우는 나이아가라 시스템(Niagara Examples Pack). BP_TeslaCoil은 GAS 상태 효과(GE_Status_Parent)와 마네킹까지 끌고 와서, 그 구성(타워 메시, 구체, 시스템 둘)만 옮겨 ALabOrb에서 같은 배치로 만든다.
    "orbfx": [
        "/Game/NiagaraExamples/FX_Ribbons/NS_TeslaCoil",       # 오브의 번개
        "/Game/NiagaraExamples/FX_Misc/NS_Boundary_Cylinder",  # 피해 범위 표시(보라색 원기둥)
        "/Game/NiagaraExamples/Gallery/StaticMesh/SM_TorchBase",  # 타워 받침대
        "/Game/NiagaraExamples/Gallery/Materials/M_Target_Metal", # 타워·구체 머티리얼
        "/Game/NiagaraExamples/Materials/MI_Mesh_Overlay_TeslaCoil", # 구체의 보라색 오버레이
    ],
}
SEEDS = SETS[os.environ.get("LAB_MIGRATE_SET", "minion")]
DEST = os.environ["LAB_MIGRATE_DEST"]
DRY = os.environ.get("LAB_MIGRATE_DRY", "1") == "1"

reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.search_all_assets(True)
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)

seen = set()
stack = list(SEEDS)
while stack:
    p = stack.pop()
    if p in seen or not p.startswith("/Game/"):
        continue
    seen.add(p)
    for d in reg.get_dependencies(p, opts) or []:
        stack.append(str(d))

content = unreal.Paths.project_content_dir()
total = 0
for p in sorted(seen):
    base = os.path.join(content, p[len("/Game/"):])
    sz = sum(os.path.getsize(base + ext) for ext in (".uasset", ".uexp", ".ubulk", ".uptnl") if os.path.exists(base + ext))
    total += sz
    unreal.log_warning("LABMIG {:>10} {}".format(sz, p))
unreal.log_warning("LABMIG TOTAL {} packages, {:.1f} MB".format(len(seen), total / 1048576.0))

if not DRY:
    mo = unreal.MigrationOptions()
    mo.set_editor_property("prompt", False)
    unreal.AssetToolsHelpers.get_asset_tools().migrate_packages(SEEDS, DEST, mo)
    unreal.log_warning("LABMIG DONE")
