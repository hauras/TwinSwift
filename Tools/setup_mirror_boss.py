# 보스 3 거울 기사 몸 (단계 3-3: 몸만. 재생·전투 시작은 4단계)
# 1) GE_Mirror_DefaultAttributes: 골렘 스탯 GE 복사, 값만 (체력 500, 공격력 12)
# 2) BP_Boss_Mirror (부모 ASMirrorBoss): 기사·마법사 몸 + ABP, 칼, 1.25배, 거울 재질, 플레이어 능력 8개(넉백 제외), 이름·스탯·이동 속도·표식
# 3) L_Dev: 경기장 옆 22m에 확인용으로 하나 배치 (런 목록에는 아직 안 넣음. 6단계에서 지움)
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

BOSS_DIR = "/Game/Blueprints/Boss"
BOSS_NAME = "BP_Boss_Mirror"
PLAYER_BP = "/Game/Blueprints/Characters/BP_Player"
MIRROR_MATERIAL = "/Game/Assets/Characters/Boss/Mirror/Materials/M_MirrorBody"
STATS_SOURCE = "/Game/Abilities/Boss/GE_Golem_DefaultAttributes"
STATS_GE = "/Game/Abilities/Boss/GE_Mirror_DefaultAttributes"

MIRROR_STATS = {"MaxHealth": 500.0, "Health": 500.0, "AttackPower": 12.0}

# 플레이어보다 조금 크게 (같은 모습이지만 보스로 보이게)
SCALE = 1.25

# 맞아서 켜지는 반응 능력은 뺌 (기록에도 안 남음)
EXCLUDED_ABILITIES = ["GA_Knockback_C"]

PLACE_OFFSET = unreal.Vector(0.0, 2200.0, 0.0)

# ---------- 스탯 GE ----------
if not eal.does_asset_exist(STATS_GE):
    eal.duplicate_asset(STATS_SOURCE, STATS_GE)
stats_bp = unreal.load_asset(STATS_GE)
stats_cdo = unreal.get_default_object(stats_bp.generated_class())
modifiers = list(stats_cdo.get_editor_property("modifiers"))
for modifier in modifiers:
    text = modifier.export_text()
    attribute = text.split('AttributeName="')[1].split('"')[0]
    if attribute in MIRROR_STATS:
        head, tail = text.split("ScalableFloatMagnitude=(Value=", 1)
        old_value, rest = tail.split(",", 1)
        modifier.import_text(f"{head}ScalableFloatMagnitude=(Value={MIRROR_STATS[attribute]:.6f},{rest}")
stats_cdo.set_editor_property("modifiers", modifiers)
unreal.BlueprintEditorLibrary.compile_blueprint(stats_bp)
eal.save_loaded_asset(stats_bp)

# ---------- 보스 BP ----------
boss_path = f"{BOSS_DIR}/{BOSS_NAME}"
if eal.does_asset_exist(boss_path):
    boss_bp = unreal.load_asset(boss_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SMirrorBoss"))
    boss_bp = tools.create_asset(BOSS_NAME, BOSS_DIR, unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
boss = unreal.get_default_object(boss_bp.generated_class())

# 플레이어 설정을 그대로 가져옴 (같은 모습·같은 능력)
player = unreal.get_default_object(unreal.load_asset(PLAYER_BP).generated_class())
player_mesh = player.get_editor_property("mesh")
player_mage = next(c for c in player.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == "MageMesh")
player_weapon = next(c for c in player.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == "WeaponMesh")
player_capsule = player.get_editor_property("capsule_component")

mirror_material = unreal.load_asset(MIRROR_MATERIAL)

capsule_radius = player_capsule.get_editor_property("capsule_radius") * SCALE
capsule_half_height = player_capsule.get_editor_property("capsule_half_height") * SCALE
capsule = boss.get_editor_property("capsule_component")
capsule.set_editor_property("capsule_radius", capsule_radius)
capsule.set_editor_property("capsule_half_height", capsule_half_height)

# 기사 몸: 발이 캡슐 바닥에 닿게 (플레이어 -87.3 / 88 비율을 키운 캡슐에 맞춤)
player_z = player_mesh.get_editor_property("relative_location").z
mesh_z = player_z / player_capsule.get_editor_property("capsule_half_height") * capsule_half_height
mesh = boss.get_editor_property("mesh")
mesh.set_editor_property("skeletal_mesh_asset", player_mesh.get_editor_property("skeletal_mesh_asset"))
mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
mesh.set_editor_property("anim_class", player_mesh.get_editor_property("anim_class"))
mesh.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, mesh_z))
mesh.set_editor_property("relative_rotation", player_mesh.get_editor_property("relative_rotation"))
mesh.set_editor_property("relative_scale3d", unreal.Vector(SCALE, SCALE, SCALE))

# 마법사 몸: 자리는 OnConstruction이 기사 몸에 맞춤 (여기서는 메시·ABP만)
mage = next(c for c in boss.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == "MageMesh")
mage.set_editor_property("skeletal_mesh_asset", player_mage.get_editor_property("skeletal_mesh_asset"))
mage.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
mage.set_editor_property("anim_class", player_mage.get_editor_property("anim_class"))

# 칼: 기사 몸의 자식이라 크기는 따라 커짐
weapon = next(c for c in boss.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == "WeaponMesh")
weapon.set_editor_property("static_mesh", player_weapon.get_editor_property("static_mesh"))
for prop in ["relative_location", "relative_rotation", "relative_scale3d"]:
    weapon.set_editor_property(prop, player_weapon.get_editor_property(prop))

# 거울 재질: 재질 칸 수만큼 덮어씀
for component in [mesh, mage]:
    slots = len(component.get_editor_property("skeletal_mesh_asset").get_editor_property("materials"))
    component.set_editor_property("override_materials", [mirror_material] * slots)
weapon_slots = len(weapon.get_editor_property("static_mesh").get_editor_property("static_materials"))
weapon.set_editor_property("override_materials", [mirror_material] * weapon_slots)

# 능력: 전사 + 마법사 (넉백 제외)
abilities = []
for key in ["warrior_config", "mage_config"]:
    for ability in player.get_editor_property(key).get_editor_property("startup_abilities"):
        if ability and ability.get_name() not in EXCLUDED_ABILITIES and ability not in abilities:
            abilities.append(ability)
boss.set_editor_property("startup_abilities", abilities)

boss.get_editor_property("character_movement").set_editor_property(
    "max_walk_speed",
    player.get_editor_property("character_movement").get_editor_property("max_walk_speed"))

# 표식 UI: 골렘 설정 복사 → 머리 위
golem = unreal.get_default_object(unreal.load_asset(f"{BOSS_DIR}/BP_Boss_Golem").generated_class())
mark_source = golem.get_components_by_class(unreal.SMarkIndicatorComponent)[0]
mark = boss.get_components_by_class(unreal.SMarkIndicatorComponent)[0]
for prop in ["crack_icon", "frost_icon", "widget_class", "draw_size", "space"]:
    mark.set_editor_property(prop, mark_source.get_editor_property(prop))
bounds = mesh.get_editor_property("skeletal_mesh_asset").get_bounds()
head_z = mesh_z + (bounds.origin.z + bounds.box_extent.z) * SCALE
mark.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, head_z + 35.0))

boss.set_editor_property("display_name", unreal.Text("거울 기사"))
boss.set_editor_property("default_attributes_effect", stats_bp.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
eal.save_loaded_asset(boss_bp)
print("BP:", boss_bp.get_path_name())
print(" 캡슐", capsule_radius, capsule_half_height, "/ 메시 z", mesh_z, "/ 표식 z", head_z + 35.0)
print(" 재질 칸: 기사", len(mesh.get_editor_property("override_materials")), "마법사", len(mage.get_editor_property("override_materials")), "칼", weapon_slots)
print(" 능력:", [a.get_name() for a in abilities])

# ---------- 확인용 배치 ----------
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()
placed = next((a for a in all_actors if a.get_class() == boss_bp.generated_class()), None)
if placed:
    print("이미 배치됨:", placed.get_actor_label(), placed.get_actor_location())
else:
    encounter = next(a for a in all_actors if isinstance(a, unreal.SBossEncounter))
    target = encounter.get_actor_location() + PLACE_OFFSET

    # 바닥 높이를 위에서 아래로 쏴서 찾음 → 캡슐 반 높이만큼 위에 세움
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(
        world,
        target + unreal.Vector(0.0, 0.0, 3000.0),
        target - unreal.Vector(0.0, 0.0, 3000.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False,
        [],
        unreal.DrawDebugTrace.NONE,
        True)
    floor_z = hit.to_tuple()[4].z if hit else target.z
    location = unreal.Vector(target.x, target.y, floor_z + capsule_half_height + 2.0)

    # 플레이어 시작 쪽(-X)을 봄
    placed = actors.spawn_actor_from_class(boss_bp.generated_class(), location, unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
    placed.set_actor_label("Boss_Mirror_Check")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("배치:", placed.get_actor_label(), location, "바닥", floor_z)

print("MIRROR_BOSS_SETUP_COMPLETE")
