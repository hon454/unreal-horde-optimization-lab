# 미니언 AnimBP 에셋 /Game/Lab/Anim/ABP_LabMinion을 만든다(부모 ULabMinionAnimInstance, 스켈레톤 Minion_Lane_Core_Skeleton).
# 애님 그래프(Blend Poses by ELabMinionState → 걷기/피격/죽기)는 스크립트로 만들 수 없어 에디터에서 채운다(engine-notes.md 10절).
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-anim-bp.py -unattended -nullrhi
import unreal

PATH, NAME = "/Game/Lab/Anim", "ABP_LabMinion"
if unreal.EditorAssetLibrary.does_asset_exist(PATH + "/" + NAME):
    unreal.log_warning("LABABP exists")
else:
    f = unreal.AnimBlueprintFactory()
    f.set_editor_property("target_skeleton", unreal.load_asset("/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Core_Skeleton"))
    f.set_editor_property("parent_class", unreal.load_class(None, "/Script/HordeOptLab.LabMinionAnimInstance"))
    abp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.AnimBlueprint, f)
    unreal.EditorAssetLibrary.save_loaded_asset(abp)
    unreal.log_warning("LABABP created %s" % abp)
