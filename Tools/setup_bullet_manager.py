# 보스 탄막 매니저 에셋
# 1) M_Bullet: 조명 없는 발광 재질. 인스턴스 값으로 주황·파랑. 탄 1000발이 같은 재질 하나 → 그리기 1번
# 2) BP_BulletManager (부모 ASBulletManager): 인스턴스 메시에 엔진 구 + M_Bullet
# 3) L_Dev: 전투 진행 액터(경기장 가운데) 자리에 하나 배치 (탄 위치는 월드 좌표라 매니저 위치는 상관없음)
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

MAT_DIR = "/Game/Assets/VFX/Boss"
MAT_NAME = "M_Bullet"
BP_DIR = "/Game/Blueprints/Enemy"
BP_NAME = "BP_BulletManager"
SPHERE = "/Engine/BasicShapes/Sphere"

BULLET_COLOR = unreal.LinearColor(1.0, 0.45, 0.08, 1.0)
BLUE_COLOR = unreal.LinearColor(0.1, 0.45, 1.0, 1.0)
BULLET_INTENSITY = 8.0

# ---------- 재질 ----------
mat_path = f"{MAT_DIR}/{MAT_NAME}"
if eal.does_asset_exist(mat_path):
    mat = unreal.load_asset(mat_path)
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
# 인스턴스 메시에서 쓰는 재질이라고 표시 (안 켜면 인스턴스로 그릴 때 기본 재질로 바뀜)
mat.set_editor_property("used_with_instanced_static_meshes", True)

# 색: 인스턴스마다 값 1개(PerInstanceCustomData 0번, 주황 0 / 파랑 1)로 두 색 사이를 고름 → 색이 섞여도 재질·그리기 1번
orange = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, -100)
orange.set_editor_property("parameter_name", "OrangeColor")
orange.set_editor_property("default_value", BULLET_COLOR)
blue = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 100)
blue.set_editor_property("parameter_name", "BlueColor")
blue.set_editor_property("default_value", BLUE_COLOR)
custom = MEL.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData, -700, 300)
custom.set_editor_property("data_index", 0)
lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -450, 50)
MEL.connect_material_expressions(orange, "", lerp, "A")
MEL.connect_material_expressions(blue, "", lerp, "B")
MEL.connect_material_expressions(custom, "", lerp, "Alpha")
intensity = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -450, 250)
intensity.set_editor_property("parameter_name", "Intensity")
intensity.set_editor_property("default_value", BULLET_INTENSITY)
mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 100)
MEL.connect_material_expressions(lerp, "", mul, "A")
MEL.connect_material_expressions(intensity, "", mul, "B")
MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
eal.save_loaded_asset(mat, only_if_is_dirty=False)
print("재질:", mat.get_path_name())

# ---------- BP ----------
bp_path = f"{BP_DIR}/{BP_NAME}"
if eal.does_asset_exist(bp_path):
    bp = unreal.load_asset(bp_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SBulletManager"))
    bp = tools.create_asset(BP_NAME, BP_DIR, unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo = unreal.get_default_object(bp.generated_class())
meshes = cdo.get_editor_property("bullet_meshes")
meshes.set_editor_property("static_mesh", unreal.load_asset(SPHERE))
meshes.set_material(0, mat)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)
print("BP:", bp.get_path_name(), "메시", meshes.get_editor_property("static_mesh").get_name())

# ---------- 레벨 배치 ----------
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()
existing = [a for a in all_actors if a.get_class() == bp.generated_class()]
if existing:
    print("이미 배치됨:", existing[0].get_actor_label())
else:
    encounter = next((a for a in all_actors if isinstance(a, unreal.SBossEncounter)), None)
    location = encounter.get_actor_location() if encounter else unreal.Vector(0.0, 0.0, 0.0)
    manager = actors.spawn_actor_from_class(bp.generated_class(), location)
    manager.set_actor_label("BulletManager")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("배치:", manager.get_actor_label(), location)

print("BULLET_MANAGER_SETUP_COMPLETE")
