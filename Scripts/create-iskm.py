# M5 ISKM(InstancedSkinnedMeshComponent) 에셋 만들기. 30\~200m 원거리 표현을 VAT ISM에서 ISKM으로 바꾸는 데 쓴다(plan.md "M5" 결정 7).
# 종류(소형 근접, 대형 시즈)마다 세 가지를 만든다.
#   SK_<TOKEN>ISKM   : 미니언 스켈레탈 메시의 복제본. 모든 LOD 빌드 설정에 bOptimizeForInstancing을 켠다.
#                      ASTPD는 통합 본 맵(UnifiedBoneMap)이 없는 메시를 거부한다(AnimSequenceTransformProviderData.cpp:476-481 IsValidFor,
#                      빌드 SkeletalMeshRenderData.cpp:259 BuildUnifiedBoneMap). 원본을 고치면 액터 경로의 렌더 데이터도 바뀌어
#                      이전 단계의 측정 조건이 달라지므로 복제본에만 켠다. 스켈레톤은 원본과 같아 클립을 그대로 쓴다.
#   ASTP_<TOKEN>     : UAnimSequenceTransformProviderData. SkinnedAsset = 위 복제본, Sequences = 0 걷기, 1 피격, 2 죽기(bake-vat.py ANIMS와 같은 클립·순서).
#   M_<TOKEN>ISKM    : 액터 머티리얼(bake-vat.py make_actor_material)과 같은 색 그래프. 사용처 SkeletalMesh + InstancedSkinnedMesh를 둘 다 켠다
#                      (ISKM 프록시는 두 플래그를 모두 검사한다, SkeletalMeshSceneProxy.cpp:233-240). 죽기 정도(DeadAmount, 0\~1)는
#                      컴포넌트 CustomPrimitiveData 0번 대신 인스턴스 커스텀 데이터 0번에서 읽는다. 값의 뜻은 액터와 같다.
#                      복제 메시의 머티리얼 슬롯에도 이 머티리얼을 넣는다.
# 엔진 API(확인한 위치는 Docs/Reference/engine-notes.md "ISKM 에셋"):
#   UTransformProviderDataFactory.ProviderDataClass(UnrealEd/Classes/Factories/TransformProviderFactory.h)
#   ASTPD SkinnedAsset·Sequences(Engine/Classes/Animation/AnimSequenceTransformProviderData.h:979-988, private + EditAnywhere)
#   USkeletalMeshEditorSubsystem::GetLODCount / Get·SetLodBuildSettings(SkeletalMeshEditor/Public/SkeletalMeshEditorSubsystem.h:252,306,315)
# 애니메이션 바운드는 에디터 패키지에 저장되지 않는다. 쿡할 때 CacheDerivedData가 계산해 넣고(.cpp:716-735), 에디터에서는 로드·편집 때
#   비동기로 다시 계산한다(.cpp:775-778, 837). Python에서 볼 수 없어(HasAnimationBounds는 UFUNCTION이 아니다) 이 스크립트는 확인하지 않는다.
# 결과는 Fab 에셋에서 나온 것이라 /Game/Lab/ISKM/Baked/(gitignore)에 둔다. 새 체크아웃에서는 bake-vat.py 뒤 한 번 돌린다.
# 실행: [LAB_UNIT=small|large] UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-iskm.py -unattended -nosplash -AllowCommandletRendering -stdout
#   LAB_UNIT을 비우면 두 종류를 다 만든다. LAB_ISKM_VERIFY=1이면 만들지 않고 저장된 에셋을 읽어 확인만 한다(새 프로세스에서 저장 결과 확인). -AllowCommandletRendering이 없으면 스켈레탈 메시 렌더 데이터와 바운드 계산이 돌지 않는다.
import os
import sys
import unreal

MINION = "/Game/ParagonMinions/Characters/Minions/Down_Minions"
UNITS = {
    "small": {
        "token": "LabMinion",
        "skel": MINION + "/Meshes/Minion_Lane_Melee_Dawn",
        "anims": [MINION + "/Animations/Melee/NonCombat_JogFwd",  # 0 걷기(루프)
                  MINION + "/Animations/Melee/HitReact_Front",    # 1 피격
                  MINION + "/Animations/Melee/Death_A"],          # 2 죽기
        "diffuse": MINION + "/Textures/Melee/T_Minion_Dawn_Melee_D",
        "normal": MINION + "/Textures/Melee/T_Minion_Dawn_Melee_N",
    },
    "large": {
        "token": "LabMinionLarge",
        "skel": MINION + "/Meshes/Minion_Lane_Siege_Dawn",
        "anims": [MINION + "/Animations/Siege/Jog_Fwd_NonCombat",  # 0 걷기(루프). 근접과 이름이 다르다
                  MINION + "/Animations/Siege/HitReact_Front",     # 1 피격
                  MINION + "/Animations/Siege/Death_A"],           # 2 죽기
        "diffuse": MINION + "/Textures/Siege/T_Minion_Dawn_Seige_D",  # 팩의 철자 그대로(Seige)
        "normal": MINION + "/Textures/Siege/T_Minion_Dawn_Seige_N",
    },
}
CLIP_NAMES = ["walk", "hit", "death"]
DEAD_AMOUNT_INDEX = 0  # 인스턴스 커스텀 데이터 0번 = DeadAmount(0 산 개체 색 \~ 1 죽기 색). 액터의 CustomPrimitiveData 0번과 같은 뜻
OUT = "/Game/Lab/ISKM/Baked"

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lab_unit_colors as colors

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
skel_sub = unreal.SkeletalMeshEditorSubsystem
tools = unreal.AssetToolsHelpers.get_asset_tools()
FAILED = []


def log(s):
    unreal.log_warning("LABISKM " + s)


def fresh(path):
    if eal.does_asset_exist(path):
        eal.delete_asset(path)


def new_asset(name, cls, factory):
    fresh(OUT + "/" + name)
    a = tools.create_asset(name, OUT, cls, factory)
    if not a:
        raise RuntimeError("create failed " + name)
    return a


def make_mesh(u):
    path = OUT + "/SK_" + u["token"] + "ISKM"
    fresh(path)
    mesh = eal.duplicate_asset(u["skel"], path)
    if not mesh:
        raise RuntimeError("duplicate failed " + u["skel"])
    n = skel_sub.get_lod_count(mesh)
    for i in range(n):
        bs = skel_sub.get_lod_build_settings(mesh, i)
        bs.set_editor_property("optimize_for_instancing", True)
        skel_sub.set_lod_build_settings(mesh, i, bs)  # FScopedSkeletalMeshPostEditChange로 다시 빌드한다
    flags = [skel_sub.get_lod_build_settings(mesh, i).get_editor_property("optimize_for_instancing") for i in range(n)]
    if not all(flags):
        raise RuntimeError("optimize_for_instancing not set: %s" % flags)
    log("%s mesh %s lods=%d optimize_for_instancing=%s" % (u["token"], path, n, flags))
    return mesh


def make_material(u, large):
    """bake-vat.py make_actor_material과 같은 그래프. DeadAmount만 PerInstanceCustomData 0번에서 읽는다."""
    dead_scale = colors.LARGE_DEAD_SCALE if large else 1.0
    dead_color = unreal.LinearColor(*[c * dead_scale for c in colors.DEAD_COLOR], 1.0)
    unit_tint = unreal.LinearColor(*(colors.LARGE_TINT if large else colors.SMALL_TINT), 1.0)

    mat = new_asset("M_" + u["token"] + "ISKM", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal", True)
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_SKINNED_MESH)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -200)
    base.set_editor_property("parameter_name", "Diffuse")
    base.set_editor_property("texture", unreal.load_asset(u["diffuse"]))
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 200)
    nrm.set_editor_property("parameter_name", "Normal")
    nrm.set_editor_property("texture", unreal.load_asset(u["normal"]))
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    dead_amount = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData, -1100, 600)
    dead_amount.set_editor_property("data_index", DEAD_AMOUNT_INDEX)
    dead_amount.set_editor_property("const_default_value", 0.0)  # 커스텀 데이터가 없는 인스턴스는 산 개체 색
    tint_a = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1100, 400)
    tint_a.set_editor_property("parameter_name", "UnitTint")
    tint_a.set_editor_property("default_value", unit_tint)
    dead = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1100, 500)
    dead.set_editor_property("parameter_name", "DeadColor")
    dead.set_editor_property("default_value", dead_color)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -900, 500)
    mel.connect_material_expressions(tint_a, "RGB", tint, "A")
    mel.connect_material_expressions(dead, "RGB", tint, "B")
    mel.connect_material_expressions(dead_amount, "", tint, "Alpha")
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, -200)
    mel.connect_material_expressions(base, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 300)
    rough.set_editor_property("r", 0.6)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    usage = (mat.get_editor_property("used_with_skeletal_mesh"), mat.get_editor_property("used_with_instanced_skinned_mesh"))
    if not all(usage):
        raise RuntimeError("material usage not set: skeletal=%s iskm=%s" % usage)
    log("%s material %s/%s usage skeletal=%s iskm=%s dead_amount=PerInstanceCustomData[%d] dead=%s tint=%s"
        % (u["token"], OUT, mat.get_name(), usage[0], usage[1], DEAD_AMOUNT_INDEX, dead_color, unit_tint))
    return mat


def set_mesh_material(mesh, mat):
    # 배열 원소는 복사본이라 고친 원소로 새 배열을 만들어 통째로 넣는다(제자리 수정은 저장되지 않았다, 2026-11-11 확인 실행)
    new = []
    for m in mesh.get_editor_property("materials"):
        m.set_editor_property("material_interface", mat)
        new.append(m)
    mesh.set_editor_property("materials", new)
    slots = [m.get_editor_property("material_interface").get_name() for m in mesh.get_editor_property("materials")]
    if any(n != mat.get_name() for n in slots):
        raise RuntimeError("mesh material not set: %s" % slots)
    log("%s material slots=%s" % (mesh.get_name(), slots))


def make_provider(u, mesh):
    factory = unreal.TransformProviderDataFactory()
    factory.set_editor_property("provider_data_class", unreal.AnimSequenceTransformProviderData)
    ast = new_asset("ASTP_" + u["token"], unreal.AnimSequenceTransformProviderData, factory)
    seqs = []
    for p in u["anims"]:
        anim = unreal.load_asset(p)
        if not anim:
            raise RuntimeError("missing anim " + p)
        s = unreal.AnimSequenceTransformProviderSequence()
        s.set_editor_property("sequence", anim)
        s.set_editor_property("play_rate", 1.0)
        s.set_editor_property("position", 0.0)
        seqs.append(s)
    ast.set_editor_property("skinned_asset", mesh)
    ast.set_editor_property("sequences", seqs)  # private + EditAnywhere + BlueprintReadOnly. set_editor_property는 CPF_Edit만 본다
    eal.save_loaded_asset(ast)

    # 저장 뒤 다시 읽어 확인
    got = ast.get_editor_property("sequences")
    names = [s.get_editor_property("sequence").get_path_name() if s.get_editor_property("sequence") else None for s in got]
    want = [unreal.load_asset(p).get_path_name() for p in u["anims"]]
    if names != want:
        raise RuntimeError("sequences mismatch: %s" % names)
    if ast.get_skinned_asset() != mesh:
        raise RuntimeError("skinned asset mismatch: %s" % ast.get_skinned_asset())
    for i, s in enumerate(got):
        anim = s.get_editor_property("sequence")
        log("%s clip %d %s %s length=%.4fs frames=%d play_length_via_astp=%.4fs"
            % (u["token"], i, CLIP_NAMES[i], anim.get_name(), anim.get_play_length(),
               unreal.AnimationLibrary.get_num_frames(anim), ast.get_sequence_play_length(i)))
    log("%s provider %s/%s skinned_asset=%s layers=%d"
        % (u["token"], OUT, ast.get_name(), mesh.get_path_name(), ast.get_num_layers()))
    return ast


def validate(assets):
    try:
        val = unreal.get_editor_subsystem(unreal.EditorValidatorSubsystem)
        settings = unreal.ValidateAssetsSettings()
        settings.set_editor_property("skip_excluded_directories", False)
        settings.set_editor_property("show_if_no_failures", True)
        datas = [unreal.AssetRegistryHelpers.get_asset_registry().get_asset_by_object_path(a.get_path_name()) for a in assets]
        ret = val.validate_assets_with_settings(datas, settings)  # 출력 인자 OutResults는 Python에서 반환값 튜플로 온다
        results = ret[1] if isinstance(ret, tuple) else ret
        log("validate checked=%d failed=%d warnings=%d valid=%d"
            % (results.get_editor_property("num_checked"), results.get_editor_property("num_invalid"),
               results.get_editor_property("num_warnings"), results.get_editor_property("num_valid")))
    except Exception as e:  # 검증기가 없어도 에셋 자체는 위에서 확인했다
        log("validate skipped: %s" % e)


def clear(u):
    """다시 돌릴 때 참조하는 쪽부터 지운다. ASTP가 메시를, 메시가 머티리얼을 참조해 반대 순서면 delete_asset이 거부한다."""
    for name in ["ASTP_" + u["token"], "SK_" + u["token"] + "ISKM", "M_" + u["token"] + "ISKM"]:
        path = OUT + "/" + name
        if eal.does_asset_exist(path) and not eal.delete_asset(path):
            raise RuntimeError("delete failed " + path)


def build(key):
    u = UNITS[key]
    clear(u)
    mesh = make_mesh(u)
    mat = make_material(u, key == "large")
    set_mesh_material(mesh, mat)
    eal.save_loaded_asset(mesh, False)  # only_if_is_dirty=False: set_editor_property가 패키지를 더럽히지 않아도 저장한다
    ast = make_provider(u, mesh)
    validate([ast, mesh])


def verify(key):
    """LAB_ISKM_VERIFY=1: 만들지 않고 저장된 에셋을 새 프로세스에서 읽어 확인한다."""
    u = UNITS[key]
    mesh = unreal.load_asset(OUT + "/SK_" + u["token"] + "ISKM")
    mat = unreal.load_asset(OUT + "/M_" + u["token"] + "ISKM")
    ast = unreal.load_asset(OUT + "/ASTP_" + u["token"])
    if not (mesh and mat and ast):
        raise RuntimeError("missing asset mesh=%s mat=%s astp=%s" % (mesh, mat, ast))
    flags = [skel_sub.get_lod_build_settings(mesh, i).get_editor_property("optimize_for_instancing") for i in range(skel_sub.get_lod_count(mesh))]
    slots = [m.get_editor_property("material_interface").get_name() for m in mesh.get_editor_property("materials")]
    usage = (mat.get_editor_property("used_with_skeletal_mesh"), mat.get_editor_property("used_with_instanced_skinned_mesh"))
    seqs = [s.get_editor_property("sequence").get_name() for s in ast.get_sequences()]
    lens = ["%.4f" % ast.get_sequence_play_length(i) for i in range(len(seqs))]
    ok = all(flags) and all(usage) and ast.get_skinned_asset() == mesh and len(seqs) == 3 and all(n == mat.get_name() for n in slots)
    log("%s verify ok=%s optimize_for_instancing=%s slots=%s usage=%s skinned_asset=%s sequences=%s lengths=%s"
        % (u["token"], ok, flags, slots, usage, ast.get_skinned_asset().get_name(), seqs, lens))
    if not ok:
        raise RuntimeError("verify failed")
    validate([ast, mesh, mat])


sel = os.environ.get("LAB_UNIT", "")
VERIFY = os.environ.get("LAB_ISKM_VERIFY") == "1"
for key in ([sel] if sel in UNITS else ["small", "large"]):
    try:
        verify(key) if VERIFY else build(key)
        log("%s done" % key)
    except Exception as e:
        FAILED.append(key)
        unreal.log_error("LABISKM %s failed: %s" % (key, e))
log("result failed=%s" % (FAILED or "none"))
