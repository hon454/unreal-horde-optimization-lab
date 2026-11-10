# static 변형(M0)과 M1 애니 없는 ISM이 같이 쓰는 미니언 정적 메시를 만든다. 레퍼런스 포즈 그대로다.
# 엔진 API: UAnimToTextureBPLibrary::ConvertSkeletalMeshToStaticMesh(SkeletalMesh, PackageName, LODIndex)
#   (Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Public/AnimToTextureBPLibrary.h:37)
# 결과는 Fab 에셋에서 나온 것이라 /Game/Lab/Derived/(gitignore)에 둔다. 새 체크아웃에서는 미니언 이전 뒤 이 스크립트를 다시 돌린다.
# 실행: [LAB_UNIT=large] UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/make-static-minion.py -unattended -AllowCommandletRendering
#   -nullrhi로는 실패한다(스켈레탈 메시 렌더 데이터에서 정점을 읽는다). 패키지 이름은 점 없는 경로다(MeshUtilities.cpp:680 CreatePackage).
import unreal

import os

# LAB_UNIT=large: M3b 2종의 대형(시즈 미니언) 정적 메시 SM_LabMinionLarge(Config/DefaultGame.ini [LabUnitAssets] LargeStaticMesh).
if os.environ.get("LAB_UNIT", "small") == "large":
    SRC = "/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Siege_Dawn"
    DST = "/Game/Lab/Derived/SM_LabMinionLarge"
else:
    SRC = "/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Melee_Dawn"
    DST = "/Game/Lab/Derived/SM_LabMinion"

if unreal.EditorAssetLibrary.does_asset_exist(DST):
    unreal.log_warning("LABSM exists " + DST)
else:
    skel = unreal.load_asset(SRC)
    sm = unreal.AnimToTextureBPLibrary.convert_skeletal_mesh_to_static_mesh(skel, DST, 0)
    if sm:
        unreal.EditorAssetLibrary.save_loaded_asset(sm)
        b = sm.get_bounding_box()
        unreal.log_warning("LABSM saved %s bounds=%s..%s mats=%d" % (DST, b.min, b.max, len(sm.static_materials)))
    else:
        unreal.log_error("LABSM failed")

# 대형의 M1 재질(2026-11-10 사용자 결정): 산 대형을 M2~M4와 같은 짙은 회색으로 그린다. Paragon 원본 머티리얼 대신
# bake-vat.py의 VAT·액터 머티리얼과 같은 식(Diffuse × 색조, Normal, 거칠기 0.6)의 단순 머티리얼을 만들어 슬롯 0에 넣는다.
# 색조는 파라미터가 아니라 상수다. 값은 bake-vat.py와 같은 Scripts/lab_unit_colors.py에서 읽는다. M1은 인스턴스 커스텀 데이터를 넘기지 않아 사망 색은 없다.
# 소형은 Paragon 원본 머티리얼 그대로다. 이미 있으면 지우고 다시 만든다(값을 바꾼 뒤 다시 돌리면 반영된다).
if os.environ.get("LAB_UNIT", "small") == "large":
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import lab_unit_colors
    LARGE_TINT = unreal.LinearColor(*lab_unit_colors.LARGE_TINT, 1.0)
    TEX = "/Game/ParagonMinions/Characters/Minions/Down_Minions/Textures/Siege/T_Minion_Dawn_Seige_"  # 팩의 철자 그대로(Seige)
    MAT_DIR, MAT_NAME = "/Game/Lab/Derived", "M_LabMinionLargeStatic"
    mel = unreal.MaterialEditingLibrary
    sm = unreal.load_asset(DST)
    sm.set_material(0, None)  # 지우기 전에 참조를 뗀다
    if unreal.EditorAssetLibrary.does_asset_exist(MAT_DIR + "/" + MAT_NAME):
        unreal.EditorAssetLibrary.delete_asset(MAT_DIR + "/" + MAT_NAME)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal", True)
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, -200)
    base.set_editor_property("parameter_name", "Diffuse")
    base.set_editor_property("texture", unreal.load_asset(TEX + "D"))
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 200)
    nrm.set_editor_property("parameter_name", "Normal")
    nrm.set_editor_property("texture", unreal.load_asset(TEX + "N"))
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, 0)
    tint.set_editor_property("constant", LARGE_TINT)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, -100)
    mel.connect_material_expressions(base, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 300)
    rough.set_editor_property("r", 0.6)
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    sm.set_material(0, mat)
    unreal.EditorAssetLibrary.save_loaded_asset(sm)
    unreal.log_warning("LABSM large static material %s on slot 0 of %d" % (mat.get_path_name(), len(sm.static_materials)))

# M1 ISM(UInstancedStaticMeshComponent)에 쓰려면 바탕 머티리얼에 bUsedWithInstancedStaticMeshes가 켜져 있어야 한다.
# 꺼져 있으면 쿠킹하지 않은 -game 실행에서 기본 머티리얼(회색)로 그려진다. 바탕 머티리얼은 Fab 에셋이라 커밋되지 않는다.
sm = unreal.load_asset(DST)
for slot in sm.static_materials:
    mi = slot.material_interface
    base = mi.get_base_material() if mi else None
    if base and not base.get_editor_property("used_with_instanced_static_meshes"):
        base.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.MaterialEditingLibrary.recompile_material(base)
        unreal.EditorAssetLibrary.save_loaded_asset(base)
        unreal.log_warning("LABSM ism usage on " + base.get_path_name())
    elif base:
        unreal.log_warning("LABSM ism usage already on " + base.get_path_name())
