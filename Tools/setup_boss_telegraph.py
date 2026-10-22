# 보스 공격 바닥 예고
# 1) 재질 M_BossTelegraph: 판 중심에서의 거리·방향을 월드 좌표로 계산 → 원·부채꼴, 테두리 + 진행률만큼 차오름 + 번쩍
#    파라미터: Radius, ForwardX/Y(정면), CosHalfAngle(부채꼴 절반 각도의 cos, 원이면 -1), Progress(0~1), Flash(0/1), Color
# 2) BP_BossTelegraph (부모 ASBossTelegraph): 엔진 평면 메시 + 위 재질
# 3) 보스 공격 GA 전부의 TelegraphClass에 연결
# 다시 실행해도 됨 (재질은 노드를 지우고 다시 만듦)
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

MAT_DIR = "/Game/Assets/VFX/Boss"
MAT_NAME = "M_BossTelegraph"
BP_DIR = "/Game/Blueprints/Enemy"
BP_NAME = "BP_BossTelegraph"
GA_DIR = "/Game/Abilities/Boss"
PLANE_MESH = "/Engine/BasicShapes/Plane"

# ---------- 재질 ----------
mat_path = f"{MAT_DIR}/{MAT_NAME}"
if eal.does_asset_exist(mat_path):
    mat = unreal.load_asset(mat_path)
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

x = -1600


def node(cls, y=0):
    global x
    x += 120
    return MEL.create_material_expression(mat, cls, x, y)


def scalar(name, default, y):
    n = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1800, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", default)
    return n


def link(a, b, pin=""):
    MEL.connect_material_expressions(a, "", b, pin)


def op(cls, a, b=None, const_b=None, y=0):
    # 두 입력 연산 (b 대신 상수도 가능)
    n = node(cls, y)
    link(a, n, "A")
    if b is not None:
        link(b, n, "B")
    elif const_b is not None:
        n.set_editor_property("const_b", const_b)
    return n


def unary(cls, a, pin="", y=0):
    n = node(cls, y)
    link(a, n, pin)
    return n


def soft_step(value, scale, y):
    # (value × scale)을 0~1로 자름 → 가장자리가 1/scale 폭으로 부드러운 경계
    return unary(unreal.MaterialExpressionSaturate, op(unreal.MaterialExpressionMultiply, value, const_b=scale, y=y), y=y)


radius = scalar("Radius", 300.0, 0)
forward_x = scalar("ForwardX", 1.0, 120)
forward_y = scalar("ForwardY", 0.0, 240)
cos_half = scalar("CosHalfAngle", -1.0, 360)
progress = scalar("Progress", 0.0, 480)
flash = scalar("Flash", 0.0, 600)
color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1800, 720)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.12, 0.04, 1.0))

# 판 중심에서 이 픽셀까지 (바닥 평면, 반경으로 나눔 → 가장자리가 1)
world = node(unreal.MaterialExpressionWorldPosition)
center = node(unreal.MaterialExpressionObjectPositionWS)
rel = op(unreal.MaterialExpressionSubtract, world, center)
rel_xy = node(unreal.MaterialExpressionComponentMask)
rel_xy.set_editor_property("r", True)
rel_xy.set_editor_property("g", True)
rel_xy.set_editor_property("b", False)
rel_xy.set_editor_property("a", False)
link(rel, rel_xy)
p = op(unreal.MaterialExpressionDivide, rel_xy, radius)
length = unary(unreal.MaterialExpressionSquareRoot, op(unreal.MaterialExpressionDotProduct, p, p))

# 부채꼴: 정면과의 각도 cos가 CosHalfAngle보다 크면 안쪽 (원이면 -1이라 전부)
forward = op(unreal.MaterialExpressionAppendVector, forward_x, forward_y, y=200)
direction = unary(unreal.MaterialExpressionNormalize, p, "VectorInput", y=200)
cos_angle = op(unreal.MaterialExpressionDotProduct, direction, forward, y=200)
fan_mask = soft_step(op(unreal.MaterialExpressionSubtract, cos_angle, cos_half, y=200), 40.0, 200)

# 원 안쪽, 테두리, 진행률만큼 찬 부분
inside = soft_step(unary(unreal.MaterialExpressionOneMinus, length, y=400), 40.0, 400)
shape = op(unreal.MaterialExpressionMultiply, inside, fan_mask, y=300)
ring = soft_step(op(unreal.MaterialExpressionSubtract, length, const_b=0.92, y=500), 25.0, 500)
fill = soft_step(op(unreal.MaterialExpressionSubtract, progress, length, y=600), 40.0, 600)

# 투명도 = 모양 × (바탕 0.15 + 테두리 0.75 + 찬 부분 0.35 + 번쩍 0.6)
strength = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, ring, const_b=0.75, y=500), op(unreal.MaterialExpressionMultiply, fill, const_b=0.35, y=600), y=550)
strength = op(unreal.MaterialExpressionAdd, strength, op(unreal.MaterialExpressionMultiply, flash, const_b=0.6, y=650), y=600)
strength = op(unreal.MaterialExpressionAdd, strength, const_b=0.15, y=600)
opacity = unary(unreal.MaterialExpressionSaturate, op(unreal.MaterialExpressionMultiply, shape, strength, y=450), y=450)
MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

# 색 = Color × (2 + 번쩍 6) → 번쩍일 때 훨씬 밝게
brightness = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, flash, const_b=6.0, y=800), const_b=2.0, y=800)
emissive = op(unreal.MaterialExpressionMultiply, color, brightness, y=750)
MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
eal.save_loaded_asset(mat)
print("재질:", mat_path)

# ---------- BP ----------
bp_path = f"{BP_DIR}/{BP_NAME}"
if eal.does_asset_exist(bp_path):
    bp = unreal.load_asset(bp_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SBossTelegraph"))
    bp = tools.create_asset(BP_NAME, BP_DIR, unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo = unreal.get_default_object(bp.generated_class())
plane = cdo.get_editor_property("plane")
plane.set_editor_property("static_mesh", unreal.load_asset(PLANE_MESH))
plane.set_editor_property("override_materials", [mat])
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
print("BP:", bp_path, plane.get_editor_property("static_mesh").get_name())

# ---------- 보스 공격 GA에 연결 ----------
for path in eal.list_assets(GA_DIR, recursive=True):
    asset = unreal.load_asset(path.split(".")[0])
    if not isinstance(asset, unreal.Blueprint):
        continue
    ga_cdo = unreal.get_default_object(asset.generated_class())
    # 등장(Intro)은 피해가 없어 예고하지 않음
    if not isinstance(ga_cdo, unreal.SGA_BossAttack) or "Intro" in asset.get_name():
        continue
    ga_cdo.set_editor_property("telegraph_class", bp.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    eal.save_loaded_asset(asset)
    print("예고 연결:", asset.get_name())
