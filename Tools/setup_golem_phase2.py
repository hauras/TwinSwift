# 골렘 2페이즈 "대지의 분노" 에셋
# 1) M_GolemArmor: 갑옷 겉껍질 오버레이 재질 (가장자리가 호박색으로 빛남 = 프레넬, 천천히 맥박). Glow(1 → 0)로 남은 갑옷 표시
# 2) GA_Golem_EarthRage: 내려치기 몽타주를 팔을 든 순간까지 느리게(충전 7초) → 경기장 전체 강타
# 3) BP_Boss_Golem: 2페이즈 값 + StartupAbilities에 추가
# 4) WBP_BossHUD: 체력바 아래 ArmorText ("대지의 분노 · 표식 폭발로 갑옷 깨기 0 / 1")
# 다시 실행해도 됨 (재질은 노드를 지우고 다시 만듦, 나머지는 값만 다시 넣음)
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

MAT_DIR = "/Game/Assets/Characters/Boss/Golem/Materials"
MAT_NAME = "M_GolemArmor"
GA_DIR = "/Game/Abilities/Boss"
GA_NAME = "GA_Golem_EarthRage"
SLAM_MONTAGE = "/Game/Assets/Characters/Boss/Golem/Montages/AM_Golem_Slam"
TELEGRAPH_BP = "/Game/Blueprints/Enemy/BP_BossTelegraph"
GOLEM_SLAM_FX = "/Game/Assets/VFX/Boss/NS_GolemSlam"
BOSS_BP = "/Game/Blueprints/Enemy/BP_Boss_Golem"
HUD_PATH = "/Game/UI/WBP_BossHUD"
PARENT = unreal.load_class(None, "/Script/TwinSwift.SGA_BossAttack")

# 갑옷 색 (호박색) / 밝기
ARMOR_COLOR = unreal.LinearColor(1.0, 0.55, 0.12, 1.0)
ARMOR_INTENSITY = 6.0

# 대지의 분노 (A_Golem_Swipe: 왼손이 머리 위 1.23초 → 바닥 1.40초, 맞는 순간 1.35초)
#   1.15초(팔을 든 자세)까지 7초에 걸쳐 느리게 → 남은 0.2초는 원래 속도로 내려침. 강타까지 약 7.2초
# 피해: 공격력 15 × 6 = 90 (전사는 버팀, 마법사 90이면 쓰러짐). 회피 무적으로는 피할 수 있음
RAGE_PROPS = {
    "montage_play_rate": 1.0,
    "charge_until_time": 1.15,
    "charge_duration": 7.0,
    "hit_times": [1.35],
    "shape": unreal.BossAttackShape.CIRCLE,
    "forward_offset": 0.0,
    # 경기장(60m) 어디에 있어도 맞게
    "radius": 6000.0,
    "damage_multiplier": 6.0,
    "cooldown_duration": 0.0,
    "hit_stun_duration": 0.0,
    "impact_effect_scale": 3.0,
    "impact_shake_outer_radius": 8000.0,
}

# 보스 2페이즈 값 (나머지는 C++ 기본값: 체력 40%, 1번, 그로기 4초, 사망 몽타주 0.8초에서 멈춤, 속도 ×1.3)
BOSS_PROPS = {
    "phase_two_health_ratio": 0.4,
    "armor_hits_required": 1,
    "groggy_duration": 4.0,
    "groggy_pose_time": 0.8,
    "phase_two_speed_scale": 1.3,
    "armor_break_effect_scale": 2.0,
    "armor_break_effect_height": 250.0,
}


def tag_container(*names):
    c = unreal.GameplayTagContainer()
    inner = ",".join(f'(TagName="{n}")' for n in names)
    c.import_text(f"(GameplayTags=({inner}))")
    return c


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text(f'(TagName="{name}")')
    return tag


# ---------- 1) 갑옷 오버레이 재질 ----------
mat_path = f"{MAT_DIR}/{MAT_NAME}"
if eal.does_asset_exist(mat_path):
    mat = unreal.load_asset(mat_path)
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

# 더하기 블렌드 + 조명 없음: 원래 몸 위에 빛만 더해짐 (오버레이 재질은 메시를 한 번 더 그림)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", False)


def node(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def scalar_param(name, value, x, y):
    n = node(unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def multiply(a, b, x, y, const_b=None):
    n = node(unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(a, "", n, "A")
    if b is not None:
        MEL.connect_material_expressions(b, "", n, "B")
    else:
        n.set_editor_property("const_b", const_b)
    return n


def add_const(a, value, x, y):
    n = node(unreal.MaterialExpressionAdd, x, y)
    MEL.connect_material_expressions(a, "", n, "A")
    n.set_editor_property("const_b", value)
    return n


color = node(unreal.MaterialExpressionVectorParameter, -1200, 0)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", ARMOR_COLOR)

# 가장자리일수록 1 (몸 윤곽이 빛나는 껍질)
fresnel = node(unreal.MaterialExpressionFresnel, -1200, 200)
fresnel.set_editor_property("exponent", 2.5)
fresnel.set_editor_property("base_reflect_fraction", 0.0)

intensity = scalar_param("Intensity", ARMOR_INTENSITY, -1200, 400)

# Glow 1 → 0 (남은 갑옷 비율, 코드가 넣음) → 밝기 1 → 0.25 (다 깨지기 직전에도 보임)
glow = scalar_param("Glow", 1.0, -1200, 550)
glow_scaled = add_const(multiply(glow, None, -1000, 550, const_b=0.75), 0.25, -850, 550)

# 맥박: 0.75 ~ 1.0 (초당 약 0.6번)
time = node(unreal.MaterialExpressionTime, -1200, 750)
sine = node(unreal.MaterialExpressionSine, -850, 750)
MEL.connect_material_expressions(multiply(time, None, -1000, 750, const_b=4.0), "", sine, "")
pulse = add_const(multiply(sine, None, -700, 750, const_b=0.125), 0.875, -550, 750)

rim = multiply(color, fresnel, -900, 100)
rim = multiply(rim, intensity, -700, 200)
rim = multiply(rim, glow_scaled, -500, 350)
rim = multiply(rim, pulse, -300, 450)
MEL.connect_material_property(rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
eal.save_loaded_asset(mat, only_if_is_dirty=False)
print("재질:", mat.get_path_name())

# ---------- 2) 대지의 분노 GA ----------
ga_path = f"{GA_DIR}/{GA_NAME}"
if eal.does_asset_exist(ga_path):
    ga_bp = unreal.load_asset(ga_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", PARENT)
    ga_bp = tools.create_asset(GA_NAME, GA_DIR, unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
ga = unreal.get_default_object(ga_bp.generated_class())

ga.set_editor_property("attack_montage", unreal.load_asset(SLAM_MONTAGE))
for key, value in RAGE_PROPS.items():
    ga.set_editor_property(key, value)
ga.set_editor_property("impact_effect", unreal.load_asset(GOLEM_SLAM_FX))
ga.set_editor_property("telegraph_class", unreal.load_asset(TELEGRAPH_BP).generated_class())
ga.set_editor_property("cooldown_tags", unreal.GameplayTagContainer())
for prop in ("asset_tags", "ability_tags"):
    try:
        ga.set_editor_property(prop, tag_container("Ability.Boss.Golem.EarthRage"))
        break
    except Exception:
        continue

# 작은 경직(State.Stun)·공격 중이어도 시작해야 함 (체력 40%가 되는 순간 플레이어가 때리는 중일 수 있음)
# 사망·큰 경직만 막음 (기본값은 경직·공격 중도 막음)
ga.set_editor_property("activation_blocked_tags", tag_container("State.Dead", "State.Groggy"))

unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
eal.save_loaded_asset(ga_bp)
print(GA_NAME, "충전", ga.get_editor_property("charge_until_time"), "초까지", ga.get_editor_property("charge_duration"), "초,",
      "맞는 시간", list(ga.get_editor_property("hit_times")), "반경", ga.get_editor_property("radius"),
      "막는 태그", ga.get_editor_property("activation_blocked_tags").export_text())

# ---------- 3) 보스 BP ----------
boss_bp = unreal.load_asset(BOSS_BP)
boss = unreal.get_default_object(boss_bp.generated_class())
abilities = list(boss.get_editor_property("startup_abilities"))
if ga_bp.generated_class() not in abilities:
    abilities.append(ga_bp.generated_class())
boss.set_editor_property("startup_abilities", abilities)
boss.set_editor_property("rage_ability_tag", make_tag("Ability.Boss.Golem.EarthRage"))
boss.set_editor_property("armor_overlay_material", mat)
boss.set_editor_property("armor_break_effect", unreal.load_asset(GOLEM_SLAM_FX))
for key, value in BOSS_PROPS.items():
    boss.set_editor_property(key, value)
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
eal.save_loaded_asset(boss_bp)
print("BP_Boss_Golem StartupAbilities:", [c.get_name() for c in boss.get_editor_property("startup_abilities")])
print("2페이즈:", {k: boss.get_editor_property(k) for k in BOSS_PROPS})

# ---------- 4) HUD 갑옷 안내 ----------
lib = unreal.TSWidgetEditLibrary
hud = unreal.load_asset(HUD_PATH)
base_font = lib.find_widget(hud, "BossNameText").get_editor_property("font")
font = unreal.SlateFontInfo()
font.import_text(base_font.export_text())
font.set_editor_property("size", 20)

armor_text = lib.add_child_widget(hud, "BossPanel", unreal.TextBlock, "ArmorText")
armor_text.set_editor_property("text", unreal.Text("대지의 분노 · 표식 폭발로 갑옷 깨기 0 / 1"))
armor_text.set_editor_property("font", font)
armor_text.set_editor_property("justification", unreal.TextJustify.CENTER)
armor_text.set_editor_property("color_and_opacity", unreal.SlateColor(ARMOR_COLOR))
armor_slot = armor_text.get_editor_property("slot")
armor_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
armor_slot.set_padding(unreal.Margin(0.0, 8.0, 0.0, 0.0))
armor_text.set_visibility(unreal.SlateVisibility.COLLAPSED)

# 글자 한 줄이 더 들어가게 패널 높이를 늘림
panel_slot = lib.find_widget(hud, "BossPanel").get_editor_property("slot")
panel_slot.set_size(unreal.Vector2D(760.0, 120.0))

lib.mark_modified(hud)
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
eal.save_loaded_asset(hud, only_if_is_dirty=False)
print([l for l in lib.dump_widget_tree(hud).splitlines() if "Armor" in l or "BossPanel" in l])

print("GOLEM_PHASE2_SETUP_COMPLETE")
