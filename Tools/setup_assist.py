# 교체 어시스트: 반투명 분신 머티리얼 + 전사 분신 BP + BP_Player에 지정
#   M_AssistGhost: 반투명·무광, 가장자리(프레넬)가 밝은 하늘색 → "분신"으로 보이게
#   BP_SwordWave (부모 ASProjectile): 금색 반달(NE_attack05)이 바닥과 평평하게 날아가는 관통 검기
#   BP_Assist_Warrior (부모 ASAssistActor): 몸·칼은 BP_Player에서 읽어 와 같은 메시, 가로 베기 한 번에 검기 발사
#   BP_Player.WarriorConfig.AssistClass = BP_Assist_Warrior
#   BP_Assist_Mage (유도 마탄 3연사, 마탄 BP_AssistDarkSolo) → BP_Player.MageConfig.AssistClass
# 조건: ASAssistActor가 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

# PIE 중에는 에셋 존재 확인(does_asset_exist)이 False를 돌려줘서 있는 에셋을 다시 만들려다 실패함
if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError("PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

GHOST = "/Game/Assets/VFX/Common/Assist/M_AssistGhost"
ASSIST_BP = "/Game/Blueprints/Assist/BP_Assist_Warrior"
PLAYER_BP = "/Game/Blueprints/Characters/BP_Player"
SWORD_WAVE_BP = "/Game/Blueprints/Projectile/BP_SwordWave"
# 콤보 마무리와 같은 금색 반달. 밝게 쓸고 지나가는 건 처음 0.17초뿐 → 날아가는 동안 WAVE_REPEAT초마다 다시 띄움
SWORD_WAVE_FX = "/Game/Assets/External/VFX/Knife_light/VFX/NE_attack05"
# 콤보 2타와 같은 가로 베기. 0.73~1.13이 실제 베는 구간 (setup_slash_fx.py 측정)
SLASH_ANIM = "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Slash_01"

# 검기: 초속 9m, 0.9초 = 약 8m (초속 16m는 너무 빨라 반달이 안 보임). 반달 반지름 100 × 1.5 = 150cm 폭으로 쓸고 감
# 반달 이펙트 몸체 수명이 1초라 0.9초 비행 동안 계속 보임
WAVE_SPEED = 900.0
WAVE_LIFE = 0.9
# 1.5배는 쿼터뷰에서 너무 작아 보임 → 2.5배 (폭 약 250cm)
# 판정 구는 발사 높이(WAVE_HEIGHT)보다 작아야 함: 바닥에 닿으면 벽으로 보고 생성 즉시 멈춤 (110/90에서 검기가 안 나감)
WAVE_SCALE = 2.5
WAVE_HIT_RADIUS = 80.0
WAVE_HEIGHT = 100.0
WAVE_REPEAT = 0.12

# 분신 색 (하늘색, HDR 세기 2) / 가운데 투명도 / 가장자리 투명도
GHOST_COLOR = unreal.LinearColor(0.35, 0.75, 1.0, 1.0)
GHOST_EMISSIVE = 2.0
CENTER_OPACITY = 0.15
EDGE_OPACITY = 0.7


def split(path):
    return path.rsplit("/", 1)


# 1. 반투명 머티리얼 (없을 때만 만듦)
if not eal.does_asset_exist(GHOST):
    folder, name = split(GHOST)
    mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", False)

    color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
    color.set_editor_property("parameter_name", "GhostColor")
    color.set_editor_property("default_value", GHOST_COLOR)
    strength = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 150)
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", GHOST_EMISSIVE)
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 50)
    MEL.connect_material_expressions(color, "", mul, "A")
    MEL.connect_material_expressions(strength, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # 투명도: 가운데 CENTER → 가장자리 EDGE (프레넬로 섞음)
    fresnel = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -600, 300)
    lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -350, 300)
    lerp.set_editor_property("const_a", CENTER_OPACITY)
    lerp.set_editor_property("const_b", EDGE_OPACITY)
    MEL.connect_material_expressions(fresnel, "", lerp, "Alpha")
    MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_OPACITY)

    MEL.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)
ghost = unreal.load_asset(GHOST)
print("ghost", ghost.get_path_name())

# 2. 플레이어의 전사 몸·칼 메시 읽기 (경로를 짐작하지 않음)
player_cdo = unreal.get_default_object(unreal.load_asset(PLAYER_BP).generated_class())
body_mesh = player_cdo.get_editor_property("mesh").get_editor_property("skeletal_mesh_asset")
weapon_comp = player_cdo.get_editor_property("weapon_mesh")
weapon_mesh = weapon_comp.get_editor_property("static_mesh")
print("body", body_mesh.get_path_name() if body_mesh else None,
      "weapon", weapon_mesh.get_path_name() if weapon_mesh else None)

# 3. 검기 투사체 BP (없을 때만 만듦)
if not eal.does_asset_exist(SWORD_WAVE_BP):
    folder, name = split(SWORD_WAVE_BP)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.SProjectile)
    tools.create_asset(name, folder, unreal.Blueprint, factory)
wave_bp = unreal.load_asset(SWORD_WAVE_BP)
wave = unreal.get_default_object(wave_bp.generated_class())
wave.set_editor_property("pierce", True)
wave.set_editor_property("max_life_time", WAVE_LIFE)
wave.set_editor_property("flight_effect_repeat_interval", WAVE_REPEAT)
movement = wave.get_editor_property("movement")
movement.set_editor_property("initial_speed", WAVE_SPEED)
movement.set_editor_property("max_speed", WAVE_SPEED)
# 검기 폭만큼 판정 (기본 12cm는 화염탄용)
wave.get_editor_property("collision").set_editor_property("sphere_radius", WAVE_HIT_RADIUS)
flight = wave.get_editor_property("flight_effect")
flight.set_editor_property("asset", unreal.load_asset(SWORD_WAVE_FX))
# 반달(SM_slash)은 XY 평면 원이고 가운데가 -X → 180도 돌려 볼록한 쪽이 날아가는 앞(+X)으로. 바닥과 평평 그대로
flight.set_editor_property("relative_rotation", unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
flight.set_editor_property("relative_scale3d", unreal.Vector(WAVE_SCALE, WAVE_SCALE, WAVE_SCALE))
unreal.BlueprintEditorLibrary.compile_blueprint(wave_bp)
eal.save_loaded_asset(wave_bp, only_if_is_dirty=False)
print("wave", SWORD_WAVE_BP)

# 4. 전사 분신 BP (없을 때만 만듦)
if not eal.does_asset_exist(ASSIST_BP):
    folder, name = split(ASSIST_BP)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.SAssistActor)
    tools.create_asset(name, folder, unreal.Blueprint, factory)
assist_bp = unreal.load_asset(ASSIST_BP)
cdo = unreal.get_default_object(assist_bp.generated_class())
body = cdo.get_editor_property("body")
body.set_editor_property("skeletal_mesh_asset", body_mesh)
weapon = cdo.get_editor_property("weapon")
weapon.set_editor_property("static_mesh", weapon_mesh)
# 칼 위치·회전·크기 보정도 플레이어와 같게 (기사 루트 뼈 스케일 보정이 들어 있음)
weapon.set_editor_property("relative_location", weapon_comp.get_editor_property("relative_location"))
weapon.set_editor_property("relative_rotation", weapon_comp.get_editor_property("relative_rotation"))
weapon.set_editor_property("relative_scale3d", weapon_comp.get_editor_property("relative_scale3d"))
cdo.set_editor_property("ghost_material", ghost)
# 가로 베기: 준비 동작(0~0.5)을 건너뛰고 0.5초부터 → 베는 구간 한가운데(0.93)가 생성 후 0.43초. 그때 검기 발사
# 베기가 끝나는 1.25초쯤(생성 후 0.75초) 사라짐
cdo.set_editor_property("assist_animation", unreal.load_asset(SLASH_ANIM))
cdo.set_editor_property("anim_start_time", 0.5)
cdo.set_editor_property("hit_times", [0.43])
cdo.set_editor_property("damage_multiplier", 1.2)
cdo.set_editor_property("life_time", 0.75)
cdo.set_editor_property("projectile_class", wave_bp.generated_class())
cdo.set_editor_property("projectile_height", WAVE_HEIGHT)
unreal.BlueprintEditorLibrary.compile_blueprint(assist_bp)
eal.save_loaded_asset(assist_bp, only_if_is_dirty=False)
print("assist", ASSIST_BP)

# 5. BP_Player 전사 설정에 어시스트 지정 (구조체는 복사본이라 고친 뒤 다시 넣음)
player_bp = unreal.load_asset(PLAYER_BP)
player_cdo = unreal.get_default_object(player_bp.generated_class())
config = player_cdo.get_editor_property("warrior_config")
config.set_editor_property("assist_class", assist_bp.generated_class())
player_cdo.set_editor_property("warrior_config", config)
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
print("player warrior assist", player_cdo.get_editor_property("warrior_config").get_editor_property("assist_class"))

# ---------- 마법사 어시스트: 유도 마탄 3연사 ----------
# 마탄 = 보라 어둠 구체 BP_AssistDarkSolo (NS_Dark_Solo 계열, 별도 스크립트로 만듦. 크기·이펙트는 그 BP에서)
MAGE_PROJECTILE_BP = "/Game/Blueprints/Projectile/BP_AssistDarkSolo"
MAGE_ASSIST_BP = "/Game/Blueprints/Assist/BP_Assist_Mage"
# 화염탄(_02)·얼음 쇄도(_01)가 안 쓰는 한 손 공격. 오른손이 0.7~0.9 앞으로 뻗고(0.9에 가장 멀리) 1.0~1.3 앞에 둔 채 들어 올림 (1/60초 실측)
MAGE_ANIM = "/Game/Assets/External/Characters/Wizard_Free/Animations/A_Wizard_1HMagicAttack_03"

mage_projectile_bp = unreal.load_asset(MAGE_PROJECTILE_BP)
if not mage_projectile_bp:
    raise RuntimeError("마탄 BP 없음: " + MAGE_PROJECTILE_BP)

# 4-2. 마법사 분신 BP (몸은 BP_Player의 마법사 몸, 무기 없음)
mage_mesh = player_cdo.get_editor_property("mage_mesh").get_editor_property("skeletal_mesh_asset")
if not eal.does_asset_exist(MAGE_ASSIST_BP):
    folder, name = split(MAGE_ASSIST_BP)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.SAssistActor)
    tools.create_asset(name, folder, unreal.Blueprint, factory)
mage_assist_bp = unreal.load_asset(MAGE_ASSIST_BP)
mcdo = unreal.get_default_object(mage_assist_bp.generated_class())
mcdo.get_editor_property("body").set_editor_property("skeletal_mesh_asset", mage_mesh)
mcdo.set_editor_property("ghost_material", ghost)
# 준비 동작(0~0.4)을 건너뛰고 0.4초부터 → 손이 앞에 있는 0.85 / 1.0 / 1.15초 = 생성 후 0.45 / 0.6 / 0.75초에 한 발씩
# 각 0.4배 (합계 1.2배, 전사 검기와 같음). 손을 거두기 시작하는 1.5초쯤(생성 후 1.1초) 사라짐
mcdo.set_editor_property("assist_animation", unreal.load_asset(MAGE_ANIM))
mcdo.set_editor_property("anim_start_time", 0.4)
mcdo.set_editor_property("hit_times", [0.45, 0.6, 0.75])
mcdo.set_editor_property("damage_multiplier", 0.4)
mcdo.set_editor_property("life_time", 1.1)
mcdo.set_editor_property("projectile_class", mage_projectile_bp.generated_class())
mcdo.set_editor_property("projectile_height", 110.0)
mcdo.set_editor_property("homing_to_nearest", True)
unreal.BlueprintEditorLibrary.compile_blueprint(mage_assist_bp)
eal.save_loaded_asset(mage_assist_bp, only_if_is_dirty=False)
print("mage assist", MAGE_ASSIST_BP, mage_mesh.get_path_name() if mage_mesh else None)

# 4-3. BP_Player 마법사 설정에 어시스트 지정
player_cdo = unreal.get_default_object(player_bp.generated_class())
mage_config = player_cdo.get_editor_property("mage_config")
mage_config.set_editor_property("assist_class", mage_assist_bp.generated_class())
player_cdo.set_editor_property("mage_config", mage_config)
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
print("player mage assist", player_cdo.get_editor_property("mage_config").get_editor_property("assist_class"))
