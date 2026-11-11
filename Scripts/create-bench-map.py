# 벤치마크 맵 /Game/Lab/Maps/L_Bench를 엔진 Basic 템플릿(Template_Default: 평평한 바닥, 해, 하늘, 안개)으로 만든다.
# 바닥은 Q2 Niagara 배경 무리의 바깥 반경(1,000m, NS_LabCrowd User.OuterRadius)보다 20m 넓게 키운다(반쪽 길이 1,020m, 2026-11-12 사용자 결정).
# 처음에는 맵 반경 R(500m, LabConfig.h FLabScenario::MapRadius) + 20m였다. 스폰 반경은 그대로 500m다. 오브·스포너·카메라는 ALabGameMode가 실행 시 만든다.
# 바닥 머티리얼(MI_ProcGrid, 개발용 M_LabBandFloor)은 월드 좌표로 무늬를 그려 스케일을 바꿔도 칸 크기가 같다(Docs/Worklog/Q2.md 2단위).
# 하늘 구(SM_SkySphere 스케일 400, 반지름 약 16km)·해·스카이 라이트·높이 안개·SkyAtmosphere는 바닥 크기에 묶여 있지 않다.
# Open World 템플릿(지형)은 쓰지 않는다. 경사가 개체마다 이동 거리와 CharacterMovement 비용을 바꿔 실행 간 수치가 흔들린다(plan.md "평지 맵").
# VolumetricCloud는 지운다. 군중과 무관한 GPU 비용이다.
# 조명은 템플릿 기본값(해 6 lux, 피치 -49.5, 스카이 1.0) 그대로 둔다. 해 질 녘 쪽으로 낮추는 안을 시험했다가 되돌렸다(2026-11-07 사용자 결정).
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-bench-map.py -unattended -nullrhi
import unreal

PATH = "/Game/Lab/Maps/L_Bench"
TEMPLATE = "/Engine/Maps/Templates/Template_Default"
CROWD_OUTER_RADIUS = 100000.0  # NS_LabCrowd User.OuterRadius(cm)와 같게 둔다
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
        scale = (CROWD_OUTER_RADIUS + MARGIN) * 2.0 / size
        actor.set_actor_scale3d(unreal.Vector(scale, scale, 1.0))
        unreal.log_warning("LABMAP floor mesh_size=%.1f scale=%.2f" % (size, scale))

ok = les.save_current_level()
unreal.log_warning("LABMAP saved=%s %s" % (ok, PATH))
