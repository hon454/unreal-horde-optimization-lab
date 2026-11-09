# 미니언 AnimBP 에셋 /Game/Lab/Anim/ABP_LabMinion을 만든다(부모 ULabMinionAnimInstance, 스켈레톤 Minion_Lane_Core_Skeleton).
# 애님 그래프(Blend Poses by ELabMinionState → 걷기/피격/죽기)는 스크립트로 만들 수 없어 에디터에서 채운다(engine-notes.md 10절).
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-anim-bp.py -unattended -nullrhi
#
# LAB_UNIT=large: M3b 2종의 대형 ABP_LabMinionLarge를 ABP_LabMinion의 복제로 만들고 대상 스켈레톤을
# Minion_Lane_Siege_Skeleton으로 바꾼다(UAnimBlueprint::TargetSkeleton은 EditAnywhere, AnimBlueprint.h:90-91).
# 그래프(블렌드 리스트, 시작 시각 핀 연결, ResetChildOnActivate)는 복제로 그대로 온다. 시퀀스 플레이어 4개의 클립을 시즈 애니로
# 바꾸고 컴파일하는 일은 Python이 그래프 노드에 닿지 않아 Unreal MCP로 한다(Scripts/create-anim-bp-large.md).
import os
import unreal

PATH = "/Game/Lab/Anim"
MINION = "/Game/ParagonMinions/Characters/Minions/Down_Minions"
eal = unreal.EditorAssetLibrary

if os.environ.get("LAB_UNIT", "small") == "large":
    NAME = "ABP_LabMinionLarge"
    if eal.does_asset_exist(PATH + "/" + NAME):
        unreal.log_warning("LABABP exists " + NAME)
    else:
        abp = eal.duplicate_asset(PATH + "/ABP_LabMinion", PATH + "/" + NAME)
        abp.set_editor_property("target_skeleton", unreal.load_asset(MINION + "/Meshes/Minion_Lane_Siege_Skeleton"))
        eal.save_loaded_asset(abp, only_if_is_dirty=False)
        unreal.log_warning("LABABP created %s skeleton=%s" % (abp.get_path_name(), abp.get_editor_property("target_skeleton").get_path_name()))
else:
    NAME = "ABP_LabMinion"
    if eal.does_asset_exist(PATH + "/" + NAME):
        unreal.log_warning("LABABP exists")
    else:
        f = unreal.AnimBlueprintFactory()
        f.set_editor_property("target_skeleton", unreal.load_asset(MINION + "/Meshes/Minion_Lane_Core_Skeleton"))
        f.set_editor_property("parent_class", unreal.load_class(None, "/Script/HordeOptLab.LabMinionAnimInstance"))
        abp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.AnimBlueprint, f)
        unreal.EditorAssetLibrary.save_loaded_asset(abp)
        unreal.log_warning("LABABP created %s" % abp)
