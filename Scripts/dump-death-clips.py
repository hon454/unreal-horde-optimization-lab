# 죽기 클립 후보의 길이·루트 모션·뼈 위치를 JSON으로 뽑는다(2026-11-10, 대형 죽기가 공중에서 멈춘 것처럼 보이는 문제의 후보 비교).
# 그림은 Scripts/death-clip-sheet.py가 이 JSON으로 그린다(렌더링 없이 뼈대 선으로).
# 엔진 API: UAnimPoseExtensions::GetAnimPoseAtFrame / GetReferencePose / GetBonePose(World = 컴포넌트 공간)
#   (Engine/Source/Editor/AnimationBlueprintLibrary/Public/AnimPose.h)
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/dump-death-clips.py -unattended -nosplash -AllowCommandletRendering -stdout
# 결과: Saved/Lab/death_clips.json(git 밖)
import json
import os
import unreal

ROOT = "/Game/ParagonMinions/Characters/Minions/Down_Minions"
SETS = {
    "large": {"mesh": ROOT + "/Meshes/Minion_Lane_Siege_Dawn", "dir": ROOT + "/Animations/Siege", "clips": ["Death_" + c for c in "ABCDEFGH"]},
    "small": {"mesh": ROOT + "/Meshes/Minion_Lane_Melee_Dawn", "dir": ROOT + "/Animations/Melee", "clips": ["Death_" + c for c in "ABCDE"]},
}
ape = unreal.AnimPoseExtensions
opts = unreal.AnimPoseEvaluationOptions()


def pose_points(pose, names):
    out = []
    for n in names:
        t = ape.get_bone_pose(pose, n, unreal.AnimPoseSpaces.WORLD)
        out.append([round(t.translation.x, 2), round(t.translation.y, 2), round(t.translation.z, 2)])
    return out


result = {}
for unit, cfg in SETS.items():
    mesh = unreal.load_asset(cfg["mesh"])
    skeleton = mesh.get_editor_property("skeleton")
    ref = ape.get_reference_pose(skeleton)
    names = [str(n) for n in ape.get_bone_names(ref)]
    # 부모 뼈: AnimPose에는 부모를 묻는 함수가 없어 UAnimationBlueprintLibrary::FindBonePathToRoot(시퀀스, 뼈)의 둘째 항목을 쓴다
    first = unreal.load_asset(cfg["dir"] + "/" + cfg["clips"][0])
    parents = []
    for n in names:
        path_to_root = [str(b) for b in unreal.AnimationLibrary.find_bone_path_to_root(first, n)]
        parents.append(names.index(path_to_root[1]) if len(path_to_root) > 1 and path_to_root[1] in names else -1)
    entry = {"skeleton": skeleton.get_path_name(), "bones": names, "parents": parents, "ref": pose_points(ref, names), "clips": []}
    for clip in cfg["clips"]:
        path = cfg["dir"] + "/" + clip
        anim = unreal.load_asset(path)
        if not anim:
            unreal.log_error("LABDEATH missing " + path)
            continue
        anim_skel = anim.get_editor_property("skeleton") if hasattr(anim, "get_editor_property") else None
        try:
            anim_skel = unreal.AnimationLibrary.get_animation_skeleton(anim) if hasattr(unreal.AnimationLibrary, "get_animation_skeleton") else anim_skel
        except Exception:
            pass
        length = anim.get_play_length()
        keys = unreal.AnimationLibrary.get_num_frames(anim)  # 샘플 키 수(마지막 키 포함 여부는 엔진 정의 그대로)
        frames = [0, max(0, round(keys / 3)), max(0, round(keys * 2 / 3)), max(0, keys - 1)]
        poses = [pose_points(ape.get_anim_pose_at_frame(anim, f, opts), names) for f in frames]
        entry["clips"].append({
            "name": clip, "path": path,
            "skeleton": anim_skel.get_path_name() if anim_skel else "?",
            "same_skeleton": bool(anim_skel and anim_skel == skeleton),
            "length": length, "keys": keys,
            "root_motion": bool(anim.get_editor_property("enable_root_motion")),
            "frames": frames, "poses": poses,
        })
        unreal.log_warning("LABDEATH %s %s len=%.3f keys=%d rootmotion=%s same_skeleton=%s" % (
            unit, clip, length, keys, anim.get_editor_property("enable_root_motion"), anim_skel == skeleton))
    result[unit] = entry

out = os.path.join(unreal.Paths.project_saved_dir(), "Lab", "death_clips.json")
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w", encoding="utf-8") as f:
    json.dump(result, f)
unreal.log_warning("LABDEATH wrote " + out)
