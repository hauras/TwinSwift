# 보스 2 탄막 지휘자 배치 (단계 10-3 앞부분: 몸만. 패턴·AI는 다음)
# 1) GE_Conductor_DefaultAttributes: 골렘 스탯 GE 복사, 값만 (테스트 체력 500)
# 2) AM_Conductor_Death: 사망 몽타주 (끝나도 쓰러진 채로)
# 3) BP_Boss_Conductor (부모 ABossCharacter): 메시·애님 BP·캡슐·떠 있는 높이·이름·스탯·사망 몽타주
# 4) L_Dev: 골렘 경기장 옆 바닥에 배치 (전투 진행 액터와는 아직 안 묶음 → 서서 대기만)
# 애님 BP(ABP_Conductor)는 에디터에서 손으로 만듦 (파이썬으로 애님 그래프를 못 그림). 없으면 경고만
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

BASE = "/Game/Assets/Characters/Boss/Conductor"
MESH = f"{BASE}/Mesh/SK_Conductor"
ANIM_BP = f"{BASE}/ABP_Conductor"
DEATH_ANIM = f"{BASE}/Animations/A_Conductor_ReactDeathBackward"
MONTAGE_DIR = f"{BASE}/Montages"
BOSS_DIR = "/Game/Blueprints/Boss"
BOSS_NAME = "BP_Boss_Conductor"
STATS_SOURCE = "/Game/Abilities/Boss/GE_Golem_DefaultAttributes"
STATS_GE = "/Game/Abilities/Boss/GE_Conductor_DefaultAttributes"

# 스탯 (테스트용 체력. 영상용은 클리어 시간을 보고 정함). 탄 1발 = 공격력 × 배율
CONDUCTOR_STATS = {"MaxHealth": 500.0, "Health": 500.0, "AttackPower": 10.0}

# 몸 높이 약 190cm. 캡슐은 서 있는 사람 크기, 메시는 HOVER만큼 띄움 (로브 끝이 바닥에서 떠 있게)
CAPSULE_RADIUS = 45.0
CAPSULE_HALF_HEIGHT = 100.0
HOVER = 50.0

# 경기장(전투 진행 액터) 기준 배치 위치 (옆으로 22m, 골렘 경기장과 겹치지 않게)
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
    if attribute in CONDUCTOR_STATS:
        head, tail = text.split("ScalableFloatMagnitude=(Value=", 1)
        old_value, rest = tail.split(",", 1)
        modifier.import_text(f"{head}ScalableFloatMagnitude=(Value={CONDUCTOR_STATS[attribute]:.6f},{rest}")
stats_cdo.set_editor_property("modifiers", modifiers)
unreal.BlueprintEditorLibrary.compile_blueprint(stats_bp)
eal.save_loaded_asset(stats_bp)

# ---------- 사망 몽타주 ----------
death_path = f"{MONTAGE_DIR}/AM_Conductor_Death"
if eal.does_asset_exist(death_path):
    death = unreal.load_asset(death_path)
else:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", unreal.load_asset(DEATH_ANIM))
    death = tools.create_asset("AM_Conductor_Death", MONTAGE_DIR, unreal.AnimMontage, factory)
death.set_editor_property("enable_auto_blend_out", False)
eal.save_loaded_asset(death)

# ---------- 보스 BP ----------
boss_path = f"{BOSS_DIR}/{BOSS_NAME}"
if eal.does_asset_exist(boss_path):
    boss_bp = unreal.load_asset(boss_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.BossCharacter"))
    boss_bp = tools.create_asset(BOSS_NAME, BOSS_DIR, unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
boss = unreal.get_default_object(boss_bp.generated_class())

# 메시 방향은 골렘과 같은 규칙 (임포트한 메시의 앞이 +Y → 캐릭터 앞 +X로 돌림)
golem = unreal.get_default_object(unreal.load_asset(f"{BOSS_DIR}/BP_Boss_Golem").generated_class())
golem_rotation = golem.get_editor_property("mesh").get_editor_property("relative_rotation")

mesh = boss.get_editor_property("mesh")
mesh.set_editor_property("skeletal_mesh_asset", unreal.load_asset(MESH))
mesh.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -CAPSULE_HALF_HEIGHT + HOVER))
mesh.set_editor_property("relative_rotation", golem_rotation)
if eal.does_asset_exist(ANIM_BP):
    mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    mesh.set_editor_property("anim_class", unreal.load_asset(ANIM_BP).generated_class())
else:
    unreal.log_warning(f"애님 BP 없음: {ANIM_BP} (만든 뒤 이 스크립트를 다시 실행)")

# 표식 UI: 새 보스도 기존 균열·빙결 아이콘과 WBP를 명시적으로 연결
mark_source = golem.get_components_by_class(unreal.SMarkIndicatorComponent)[0]
mark = boss.get_components_by_class(unreal.SMarkIndicatorComponent)[0]
for prop in ["crack_icon", "frost_icon", "widget_class", "draw_size", "space"]:
    mark.set_editor_property(prop, mark_source.get_editor_property(prop))
bounds = mesh.get_editor_property("skeletal_mesh_asset").get_bounds()
head_z = mesh.get_editor_property("relative_location").z + (bounds.origin.z + bounds.box_extent.z) * mesh.get_editor_property("relative_scale3d").z
mark.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, head_z + 35.0))

capsule = boss.get_editor_property("capsule_component")
capsule.set_editor_property("capsule_radius", CAPSULE_RADIUS)
capsule.set_editor_property("capsule_half_height", CAPSULE_HALF_HEIGHT)

boss.set_editor_property("display_name", unreal.Text("탄막 지휘자"))
boss.set_editor_property("default_attributes_effect", stats_bp.generated_class())
boss.set_editor_property("death_montage", death)
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
eal.save_loaded_asset(boss_bp)
print("BP:", boss_bp.get_path_name(), "메시 회전", golem_rotation, "애님", mesh.get_editor_property("anim_class"))

# ---------- 레벨 배치 ----------
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
    location = unreal.Vector(target.x, target.y, floor_z + CAPSULE_HALF_HEIGHT + 2.0)

    # 플레이어 시작 쪽(-X)을 봄
    placed = actors.spawn_actor_from_class(boss_bp.generated_class(), location, unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
    placed.set_actor_label("Boss_Conductor")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("배치:", placed.get_actor_label(), location, "바닥", floor_z)

print("CONDUCTOR_BOSS_SETUP_COMPLETE")
