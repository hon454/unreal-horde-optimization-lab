# static 변형(M0)과 M1 애니 없는 ISM이 같이 쓰는 미니언 정적 메시를 만든다. 레퍼런스 포즈 그대로다.
# 엔진 API: UAnimToTextureBPLibrary::ConvertSkeletalMeshToStaticMesh(SkeletalMesh, PackageName, LODIndex)
#   (Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Public/AnimToTextureBPLibrary.h:37)
# 결과는 Fab 에셋에서 나온 것이라 /Game/Lab/Derived/(gitignore)에 둔다. 새 체크아웃에서는 미니언 이전 뒤 이 스크립트를 다시 돌린다.
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/make-static-minion.py -unattended -AllowCommandletRendering
#   -nullrhi로는 실패한다(스켈레탈 메시 렌더 데이터에서 정점을 읽는다). 패키지 이름은 점 없는 경로다(MeshUtilities.cpp:680 CreatePackage).
import unreal

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
