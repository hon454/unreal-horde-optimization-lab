# 벤치마크 맵 /Game/Lab/Maps/L_Bench를 엔진 Basic 템플릿(Template_Default: 평평한 바닥, 해, 하늘, 안개)으로 만든다.
# 바닥은 맵 반경 R(250m, LabConfig.h FLabScenario::MapRadius)보다 20m 넓게 키운다. 오브·스포너·카메라는 ALabGameMode가 실행 시 만든다.
# Open World 템플릿(지형)은 쓰지 않는다. 경사가 개체마다 이동 거리와 CharacterMovement 비용을 바꿔 실행 간 수치가 흔들린다(plan.md "평지 맵").
# VolumetricCloud는 지운다. 군중과 무관한 GPU 비용이다.
# 조명은 템플릿 한낮(해 6 lux, 피치 -49.5, 스카이 1.0)과 시험한 해 질 녘(해 0.4 lux, 피치 -6, 요 60, 스카이 0.25) 사이에서
# 사용자가 고른 "해 질 녘 -1, 한낮 1 척도의 0.4"다(2026-11-07). 자동 노출이 밝기를 보정해서 체감 밝기는 해의 높이(하늘 대기 색)가
# 정한다. 선형 70% 지점(피치 -36.5)은 한낮과 거의 같아 보여서, 해 높이를 낮춰 눈으로 맞췄다.
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-bench-map.py -unattended -nullrhi
import unreal

PATH = "/Game/Lab/Maps/L_Bench"
TEMPLATE = "/Engine/Maps/Templates/Template_Default"
MAP_RADIUS = 25000.0
MARGIN = 2000.0
SUN_INTENSITY = 2.0
SUN_ROTATION = unreal.Rotator(roll=0.0, pitch=-14.0, yaw=30.0)
SKY_INTENSITY = 0.5

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
    elif isinstance(actor, unreal.DirectionalLight):
        actor.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity", SUN_INTENSITY)
        actor.set_actor_rotation(SUN_ROTATION, False)
        unreal.log_warning("LABMAP sun %.2f lux" % SUN_INTENSITY)
    elif isinstance(actor, unreal.SkyLight):
        actor.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity", SKY_INTENSITY)
        unreal.log_warning("LABMAP sky %.3f" % SKY_INTENSITY)

ok = les.save_current_level()
unreal.log_warning("LABMAP saved=%s %s" % (ok, PATH))
