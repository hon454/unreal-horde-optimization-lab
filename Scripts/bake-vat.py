# M2 VAT(정점 애니메이션 텍스처) 굽기. 미니언의 걷기·피격·죽기를 텍스처에 굽고, 그 텍스처를 읽는 머티리얼과 정적 메시를 만든다.
# Bone 모드(대표)와 Vertex 모드(E2 (b) 메모리 비교용)를 한 벌씩 만든다.
#
# 엔진 API(Engine/Plugins/Experimental/AnimToTexture):
#   UAnimToTextureBPLibrary::ConvertSkeletalMeshToStaticMesh, AnimationToTexture, UpdateMaterialInstanceFromDataAsset (AnimToTextureEditor/Public/AnimToTextureBPLibrary.h)
#   AnimationToTexture는 데이터 에셋의 텍스처가 미리 있어야 쓴다(없으면 WriteVectorsToTexture가 false를 돌려주고 조용히 넘어간다, AnimToTextureUtils.h:126).
#   머티리얼 함수 MF_BoneAnimation / MF_VertexAnimation은 Material Attributes를 받아 WPO와 노멀을 바꿔 돌려준다.
#   AutoPlay 스위치를 끄면 인스턴스 커스텀 데이터 0 = Frame, 1 = PrevFrame을 읽고 [0, NumFrames-1]로 자른다(GetFrameSwitch, GetFrame).
#   한 번만 도는 피격·죽기가 반복 재생(AutoPlay는 fmod)으로 튀지 않게 프레임은 C++이 계산해 넘긴다.
# 결과는 Fab 에셋에서 나온 것이라 /Game/Lab/VAT/Baked/(gitignore)에 둔다. 새 체크아웃에서는 미니언 이전 뒤 한 번 돌린다.
# 실행: UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/bake-vat.py -unattended -nosplash -AllowCommandletRendering -stdout
#   (-nullrhi면 스켈레탈 메시 정점을 읽지 못한다. make-static-minion.py와 같다)
import unreal

MINION = "/Game/ParagonMinions/Characters/Minions/Down_Minions"
SKEL = MINION + "/Meshes/Minion_Lane_Melee_Dawn"
ANIMS = [MINION + "/Animations/Melee/NonCombat_JogFwd",  # 0 걷기(루프)
         MINION + "/Animations/Melee/HitReact_Front",    # 1 피격
         MINION + "/Animations/Melee/Death_A"]           # 2 죽기
DIFFUSE = MINION + "/Textures/Melee/T_Minion_Dawn_Melee_D"
NORMAL = MINION + "/Textures/Melee/T_Minion_Dawn_Melee_N"
OUT = "/Game/Lab/VAT/Baked"
A2T = "/AnimToTexture/Materials/MaterialFunctions"
DEAD_COLOR = unreal.LinearColor(0.6, 0.06, 0.05, 1.0)  # 죽기 진홍색(2026-11-08 사용자 확정). VAT·액터 머티리얼이 같은 값을 쓴다

lib = unreal.AnimToTextureBPLibrary
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(s):
    unreal.log_warning("LABVAT " + s)


def fresh(path):
    if eal.does_asset_exist(path):
        eal.delete_asset(path)


def new_asset(name, cls, factory):
    fresh(OUT + "/" + name)
    a = tools.create_asset(name, OUT, cls, factory)
    if not a:
        raise RuntimeError("create failed " + name)
    return a


def scalar_param(mat, name, value, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def const(mat, value, x, y):
    c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property("r", value)
    return c


def binary(mat, cls, a, b, x, y, a_out=""):
    e = mel.create_material_expression(mat, cls, x, y)
    mel.connect_material_expressions(a, a_out, e, "A")
    mel.connect_material_expressions(b, "", e, "B")
    return e


def make_material(mode):
    """Diffuse·Normal → Make Material Attributes → MF_*Animation → 출력. Paragon 마스터 머티리얼의 마스크·발광은 옮기지 않는다."""
    name = "M_LabMinionVAT_" + mode
    mat = new_asset(name, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("use_material_attributes", True)
    mat.set_editor_property("tangent_space_normal", True)
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)

    make = mel.create_material_expression(mat, unreal.MaterialExpressionMakeMaterialAttributes, -600, 0)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -200)
    base.set_editor_property("parameter_name", "Diffuse")
    base.set_editor_property("texture", unreal.load_asset(DIFFUSE))
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 200)
    nrm.set_editor_property("parameter_name", "Normal")
    nrm.set_editor_property("texture", unreal.load_asset(NORMAL))
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -1000, 500)
    rough.set_editor_property("r", 0.6)
    mel.connect_material_expressions(nrm, "RGB", make, "Normal")
    mel.connect_material_expressions(rough, "", make, "Roughness")

    # 죽기 표현(2026-11-08 사용자 요청): 로직은 그대로 두고 머티리얼만 바꾼다. 커스텀 데이터 0(Frame)이 죽기 클립 안이면
    # 진행도 t = saturate((Frame - DeathStart) / (DeathEnd - DeathStart))로 색을 붉게 바꾼다.
    # CPU가 넘기는 숫자는 늘지 않는다(E2에서 잰 VAT 비용은 커스텀 데이터 쓰기였다). 걷기·피격 프레임은 DeathStart보다 작아 t = 0이다.
    frame = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData, -1800, 700)
    frame.set_editor_property("data_index", 0)
    d0 = scalar_param(mat, "DeathStartFrame", 0.0, -1800, 800)
    d1 = scalar_param(mat, "DeathEndFrame", 1.0, -1800, 900)
    num = binary(mat, unreal.MaterialExpressionSubtract, frame, d0, -1600, 700)
    den = binary(mat, unreal.MaterialExpressionSubtract, d1, d0, -1600, 850)
    t = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -1300, 750)
    mel.connect_material_expressions(binary(mat, unreal.MaterialExpressionDivide, num, den, -1450, 750), "", t, "")

    # 붉게 물들이기: 진행도 0.4에 다 바뀐다. 원래 색 x 흰색 → 원래 색 x DeadColor. 땅으로 꺼지기는 시험 뒤 뺐다(2026-11-08 사용자 의견:
    # 어둡게만 하면 산 미니언과 잘 구별되지 않고, 붉은 기를 섞는 편이 낫다).
    dark = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -1100, 600)
    mel.connect_material_expressions(binary(mat, unreal.MaterialExpressionMultiply, t, const(mat, 2.5, -1300, 600), -1200, 600), "", dark, "")
    white = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1100, 400)
    white.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    dead = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1100, 500)
    dead.set_editor_property("parameter_name", "DeadColor")
    dead.set_editor_property("default_value", DEAD_COLOR)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -900, 500)
    mel.connect_material_expressions(white, "", tint, "A")
    mel.connect_material_expressions(dead, "RGB", tint, "B")
    mel.connect_material_expressions(dark, "", tint, "Alpha")
    mel.connect_material_expressions(binary(mat, unreal.MaterialExpressionMultiply, base, tint, -750, -200, "RGB"), "", make, "BaseColor")

    call = mel.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, -250, 0)
    call.set_editor_property("material_function", unreal.load_asset(A2T + "/MF_%sAnimation" % mode))
    mel.connect_material_expressions(make, "", call, "Material Attributes")
    mel.connect_material_property(call, "Result", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def bake(mode):
    sm_path = OUT + "/SM_LabMinionVAT_" + mode
    fresh(sm_path)
    sm = lib.convert_skeletal_mesh_to_static_mesh(unreal.load_asset(SKEL), sm_path, 0)
    if not sm:
        raise RuntimeError("convert failed")
    # 움직이는 ISM이라 라이트맵이 필요 없다. 라이트맵 UV 생성을 꺼서 VAT용 UV 채널 1과 겹치지 않게 한다
    # (AnimationToTexture가 겹치면 거부한다, AnimToTextureBPLibrary.cpp:418-424).
    lib.set_light_map_index(sm, 0, 2, False)

    da = new_asset("DA_LabMinionVAT_" + mode, unreal.AnimToTextureDataAsset, unreal.DataAssetFactory())
    da.set_editor_property("skeletal_mesh", unreal.load_asset(SKEL))
    da.set_editor_property("static_mesh", sm)
    da.set_editor_property("uv_channel", 1)
    da.set_editor_property("sample_rate", 30.0)
    da.set_editor_property("auto_play", False)
    da.set_editor_property("mode", getattr(unreal.AnimToTextureMode, mode.upper()))
    seqs = []
    for p in ANIMS:
        info = unreal.AnimToTextureAnimSequenceInfo()
        info.set_editor_property("enabled", True)
        info.set_editor_property("anim_sequence", unreal.load_asset(p))
        seqs.append(info)
    da.set_editor_property("anim_sequences", seqs)

    tex_names = ["BonePosition", "BoneRotation", "BoneWeight"] if mode == "Bone" else ["VertexPosition", "VertexNormal"]
    props = {"BonePosition": "bone_position_texture", "BoneRotation": "bone_rotation_texture", "BoneWeight": "bone_weight_texture",
             "VertexPosition": "vertex_position_texture", "VertexNormal": "vertex_normal_texture"}
    for t in tex_names:
        tex = new_asset("T_LabMinionVAT_%s_%s" % (mode, t), unreal.Texture2D, unreal.Texture2DFactoryNew())
        da.set_editor_property(props[t], tex)

    if not lib.animation_to_texture(da):
        raise RuntimeError("AnimationToTexture failed (see LogAnimToTextureEditor)")

    mat = make_material(mode)
    mi = new_asset("MI_LabMinionVAT_" + mode, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    lib.update_material_instance_from_data_asset(da, mi)
    death = da.get_editor_property("animations")[2]  # ANIMS 순서: 걷기, 피격, 죽기
    mel.set_material_instance_scalar_parameter_value(mi, "DeathStartFrame", float(death.start_frame))
    mel.set_material_instance_scalar_parameter_value(mi, "DeathEndFrame", float(death.end_frame))
    mel.update_material_instance(mi)
    sm.set_material(0, mi)

    for a in [sm, da, mi] + [unreal.load_asset("%s/T_LabMinionVAT_%s_%s" % (OUT, mode, t)) for t in tex_names]:
        eal.save_loaded_asset(a)

    anims = ", ".join("%d..%d" % (a.start_frame, a.end_frame) for a in da.get_editor_property("animations"))
    sizes = ", ".join("%s %dx%d" % (t, unreal.load_asset("%s/T_LabMinionVAT_%s_%s" % (OUT, mode, t)).blueprint_get_size_x(),
                                     unreal.load_asset("%s/T_LabMinionVAT_%s_%s" % (OUT, mode, t)).blueprint_get_size_y()) for t in tex_names)
    log("%s frames=%d anims=[%s] textures=[%s] verts=%d" % (mode, da.get_editor_property("num_frames"), anims, sizes,
                                                         unreal.EditorStaticMeshLibrary.get_number_verts(sm, 0)))


def make_actor_material():
    """M3 액터용. VAT 머티리얼과 같은 기본 색·노멀·거칠기와 죽기 진홍색이라 30m 전환 때 색이 튀지 않는다(plan.md "M3").
    죽기 정도(0\~1)는 C++이 컴포넌트의 커스텀 프리미티브 데이터 0번에 넣는다(ULabActorSyncProcessor)."""
    mat = new_asset("M_LabMinionActor", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal", True)
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -200)
    base.set_editor_property("parameter_name", "Diffuse")
    base.set_editor_property("texture", unreal.load_asset(DIFFUSE))
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 200)
    nrm.set_editor_property("parameter_name", "Normal")
    nrm.set_editor_property("texture", unreal.load_asset(NORMAL))
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    dead_amount = scalar_param(mat, "DeadAmount", 0.0, -1100, 600)  # 커스텀 프리미티브 데이터 0번에서 읽는다(MaterialExpressionScalarParameter.h:56-60)
    dead_amount.set_editor_property("use_custom_primitive_data", True)
    dead_amount.set_editor_property("primitive_data_index", 0)
    white = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1100, 400)
    white.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    dead = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1100, 500)
    dead.set_editor_property("parameter_name", "DeadColor")
    dead.set_editor_property("default_value", DEAD_COLOR)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -900, 500)
    mel.connect_material_expressions(white, "", tint, "A")
    mel.connect_material_expressions(dead, "RGB", tint, "B")
    mel.connect_material_expressions(dead_amount, "", tint, "Alpha")
    mel.connect_material_property(binary(mat, unreal.MaterialExpressionMultiply, base, tint, -700, -200, "RGB"), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(const(mat, 0.6, -700, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    log("actor material saved")


for m in ["Bone", "Vertex"]:
    try:
        bake(m)
    except Exception as e:
        unreal.log_error("LABVAT %s failed: %s" % (m, e))
try:
    make_actor_material()
except Exception as e:
    unreal.log_error("LABVAT actor material failed: %s" % e)
