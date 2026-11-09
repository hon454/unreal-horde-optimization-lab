# 개발 빌드용 바닥 머티리얼 /Game/Lab/Materials/M_LabBandFloor를 만든다(backlog.md 결정 6, 오브 중심 거리 띠).
# 오브 위치(BandCenter)에서 XY 거리로 0~30 / 30~80 / 80~200 / 200m~ 네 구간을 다른 색으로 칠하고, 경계 30·80·200m에 흰 선을 긋는다.
# 경계는 저격 구간 FLabSnipeBands와 같다. 10m 격자를 옅게 그려 템플릿 바닥(MI_ProcGrid)처럼 거리감을 남긴다.
# 측정 빌드는 기본 바닥을 쓰고, 코드가 -LabBandFloor일 때만 Floor에 이 머티리얼의 MID를 넣고 파라미터를 채운다.
#
# 파라미터(코드가 넣는다. 단위 cm, 월드 좌표):
#   BandCenter   (Vector)  오브 위치. XY만 쓴다. 기본 (0, 0, 0) = 맵 중앙
#   BandRadius1  (Scalar)  3000   첫 경계(액터 ↔ VAT, 저격 구간 1)
#   BandRadius2  (Scalar)  8000   둘째 경계(저격 구간 2)
#   BandRadius3  (Scalar)  20000  셋째 경계(M3부터 Off)
#   ColorNear·ColorMid·ColorFar·ColorOff (Vector) 구간 색, LineColor·LineWidth 경계선, GridSize 격자 간격
#
# 실행(에디터가 떠 있어도 된다. 새 패키지만 쓴다):
#   UnrealEditor-Cmd.exe HordeOptLab.uproject -run=pythonscript -script=Scripts/create-band-floor.py -unattended -nosplash -nullrhi -stdout
# 순서와 확인 방법은 Scripts/create-band-floor.md.
import unreal

PATH, NAME = "/Game/Lab/Materials", "M_LabBandFloor"
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# 띠 색과 격자를 한 노드에서 계산한다. 노드 수십 개로 펼친 것과 결과가 같고, 읽는 사람이 식을 한눈에 본다.
HLSL = """
float d = length(WP.xy - Center.xy);
float3 c = ColorOff;
c = d < R3 ? ColorFar : c;
c = d < R2 ? ColorMid : c;
c = d < R1 ? ColorNear : c;
float edge = max(max(1 - saturate(abs(d - R1) / LineW), 1 - saturate(abs(d - R2) / LineW)), 1 - saturate(abs(d - R3) / LineW));
float2 g = abs(frac(WP.xy / Grid + 0.5) - 0.5) * Grid;
float grid = 1 - saturate(min(g.x, g.y) / (LineW * 0.25));
float3 base = Base * (1 - 0.3 * grid);
return lerp(base * c, LineColor, edge);
"""


def log(s):
    unreal.log_warning("LABBAND " + s)


def vec(mat, name, rgb, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return p


def scalar(mat, name, value, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


if eal.does_asset_exist(PATH + "/" + NAME):
    eal.delete_asset(PATH + "/" + NAME)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())

custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, 0)
custom.set_editor_property("description", "LabBandFloor")
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("code", HLSL.strip())

# (입력 이름, 노드)
y = -600
inputs = []
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, y)
inputs.append(("WP", wp, ""))
y += 100
inputs.append(("Center", vec(mat, "BandCenter", (0.0, 0.0, 0.0), -900, y), "RGB")); y += 100
inputs.append(("R1", scalar(mat, "BandRadius1", 3000.0, -900, y), "")); y += 100
inputs.append(("R2", scalar(mat, "BandRadius2", 8000.0, -900, y), "")); y += 100
inputs.append(("R3", scalar(mat, "BandRadius3", 20000.0, -900, y), "")); y += 100
# 구간 색은 바닥 회색에 곱한다. 가까운 순서로 주황(액터), 초록, 파랑, 흰색(Off, 바닥 그대로)
inputs.append(("ColorNear", vec(mat, "ColorNear", (1.0, 0.62, 0.32), -900, y), "RGB")); y += 100
inputs.append(("ColorMid", vec(mat, "ColorMid", (0.45, 0.9, 0.5), -900, y), "RGB")); y += 100
inputs.append(("ColorFar", vec(mat, "ColorFar", (0.45, 0.6, 1.0), -900, y), "RGB")); y += 100
inputs.append(("ColorOff", vec(mat, "ColorOff", (1.0, 1.0, 1.0), -900, y), "RGB")); y += 100
inputs.append(("LineColor", vec(mat, "LineColor", (0.9, 0.9, 0.9), -900, y), "RGB")); y += 100
inputs.append(("LineW", scalar(mat, "LineWidth", 40.0, -900, y), "")); y += 100
inputs.append(("Grid", scalar(mat, "GridSize", 1000.0, -900, y), "")); y += 100
inputs.append(("Base", scalar(mat, "BaseGray", 0.35, -900, y), "")); y += 100

ci = []
for name, _, _ in inputs:
    i = unreal.CustomInput()
    i.set_editor_property("input_name", name)
    ci.append(i)
custom.set_editor_property("inputs", ci)
for name, node, out in inputs:
    if not mel.connect_material_expressions(node, out, custom, name):
        log("connect failed " + name)

rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
rough.set_editor_property("r", 0.9)
mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat)
eal.save_loaded_asset(mat)
log("saved %s params=%s" % (mat.get_path_name(), ", ".join(str(n) for n in mel.get_scalar_parameter_names(mat) + mel.get_vector_parameter_names(mat))))
