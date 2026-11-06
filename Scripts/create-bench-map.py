# 벤치마크 맵 /Game/Lab/Maps/L_Bench를 엔진 Basic 템플릿(Template_Default: 평평한 바닥, 해, 하늘, 안개)으로 만든다.
# 바닥은 맵 반경 R(250m, LabConfig.h FLabScenario::MapRadius)보다 20m 넓게 키운다. 오브·스포너·카메라는 ALabGameMode가 실행 시 만든다.
# Open World 템플릿(지형)은 쓰지 않는다. 경사가 개체마다 이동 거리와 CharacterMovement 비용을 바꿔 실행 간 수치가 흔들린다(plan.md "평지 맵").
# VolumetricCloud는 지운다. 군중과 무관한 GPU 비용이다.
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-bench-map.py -unattended -nullrhi
import unreal

PATH = "/Game/Lab/Maps/L_Bench"
TEMPLATE = "/Engine/Maps/Templates/Template_Default"
MAP_RADIUS = 25000.0
MARGIN = 2000.0

if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    unreal.EditorAssetLibrary.delete_asset(PATH)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.new_level_from_template(PATH, TEMPLATE)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in eas.get_all_level_actors():
    label = actor.get_actor_label()
    if isinstance(actor, unreal.VolumetricCloud):
        eas.destroy_actor(actor)
        unreal.log_warning("LABMAP removed " + label)
    elif isinstance(actor, unreal.StaticMeshActor) and label == "Floor":
        mesh = actor.static_mesh_component.static_mesh
        size = mesh.get_bounding_box().max.x - mesh.get_bounding_box().min.x
        scale = (MAP_RADIUS + MARGIN) * 2.0 / size
        actor.set_actor_scale3d(unreal.Vector(scale, scale, 1.0))
        unreal.log_warning("LABMAP floor mesh_size=%.1f scale=%.2f" % (size, scale))

ok = les.save_current_level()
unreal.log_warning("LABMAP saved=%s %s" % (ok, PATH))
