# 조각 3 에셋: 공격 몽타주 + 칼날 판정 구간, 능력 BP, BP_Player에 능력 지급, 허수아비 BP + 맵 배치
# 조건: GA_AttackBase·ANS_WeaponTrace·EnemyCharacter가 들어간 C++이 빌드돼 있어야 함
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
anim_lib = unreal.AnimationLibrary


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 1. 몽타주: A_Knight_Slash_03 (1.27초, 휘두름 0.47~0.80초, 피크 0.57초) ----------
# A_Knight_Attack은 0.33초에 멈칫하는 준비 동작이 있어 기본 공격으로는 느려서 교체 (2026-10-14)
montage_path = "/Game/Asset/Knight_Free/Montages/AM_Warrior_Attack"
if eal.does_asset_exist(montage_path):
    montage = load(montage_path)
else:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", load("/Game/Asset/Knight_Free/Animations/A_Knight_Slash_03"))
    montage = tools.create_asset("AM_Warrior_Attack", "/Game/Asset/Knight_Free/Montages", unreal.AnimMontage, factory)
    print("만듦:", montage_path)

# 칼날 판정 구간: 손 속도 500 이상인 구간
anim_lib.remove_animation_notify_events_by_track(montage, "Hit")
if "Hit" not in [str(n) for n in anim_lib.get_animation_notify_track_names(montage)]:
    anim_lib.add_animation_notify_track(montage, "Hit")
anim_lib.add_animation_notify_state_event(montage, "Hit", 0.45, 0.35, unreal.ANS_WeaponTrace)
eal.save_loaded_asset(montage, only_if_is_dirty=False)

# 예전 몽타주(A_Knight_Attack 기반) 정리
OLD_MONTAGE = "/Game/Asset/Knight_Free/Montages/AM_Warrior_Attack1"

# ---------- 2. 능력 BP: GA_Warrior_Attack (부모 C++ GA_AttackBase) ----------
ga_path = "/Game/Abilities/Warrior/GA_Warrior_Attack"
if eal.does_asset_exist(ga_path):
    ga_bp = load(ga_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.GA_AttackBase)
    ga_bp = tools.create_asset("GA_Warrior_Attack", "/Game/Abilities/Warrior", unreal.Blueprint, factory)
    print("만듦:", ga_path)
ga_cdo = unreal.get_default_object(ga_bp.generated_class())
ga_cdo.set_editor_property("attack_montage", montage)
ga_cdo.set_editor_property("damage_multiplier", 1.0)
unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
eal.save_loaded_asset(ga_bp, only_if_is_dirty=False)

if eal.does_asset_exist(OLD_MONTAGE):
    print("예전 몽타주 삭제:", eal.delete_asset(OLD_MONTAGE))

# ---------- 3. BP_Player 시작 능력 ----------
player_bp = load("/Game/Blueprints/BP_Player")
player_cdo = unreal.get_default_object(player_bp.generated_class())
player_cdo.set_editor_property("startup_abilities", [ga_bp.generated_class()])
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)

# ---------- 4. 허수아비 BP_Dummy (부모 C++ EnemyCharacter, 기사 메시 + Idle) ----------
dummy_path = "/Game/Blueprints/Enemy/BP_Dummy"
if eal.does_asset_exist(dummy_path):
    dummy_bp = load(dummy_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.EnemyCharacter)
    dummy_bp = tools.create_asset("BP_Dummy", "/Game/Blueprints/Enemy", unreal.Blueprint, factory)
    print("만듦:", dummy_path)
dummy_cdo = unreal.get_default_object(dummy_bp.generated_class())
mesh = dummy_cdo.get_editor_property("mesh")
player_mesh = player_cdo.get_editor_property("mesh")
mesh.set_editor_property("skeletal_mesh_asset", player_mesh.get_editor_property("skeletal_mesh_asset"))
mesh.set_editor_property("relative_location", player_mesh.get_editor_property("relative_location"))
mesh.set_editor_property("relative_rotation", player_mesh.get_editor_property("relative_rotation"))
mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
anim_data = mesh.get_editor_property("animation_data")
anim_data.set_editor_property("anim_to_play", load("/Game/Asset/Knight_Free/Animations/A_Knight_Idle"))
anim_data.set_editor_property("saved_looping", True)
anim_data.set_editor_property("saved_playing", True)
mesh.set_editor_property("animation_data", anim_data)
unreal.BlueprintEditorLibrary.compile_blueprint(dummy_bp)
eal.save_loaded_asset(dummy_bp, only_if_is_dirty=False)

# ---------- 5. L_Dev에 허수아비 배치 (플레이어 시작 위치 앞 4m) ----------
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Dummy"]
if not existing:
    dummy = eas.spawn_actor_from_class(dummy_bp.generated_class(), unreal.Vector(400, 0, 100), unreal.Rotator(roll=0, pitch=0, yaw=180))
    dummy.set_actor_label("Dummy")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("L_Dev에 Dummy 배치")

print("완료: 몽타주, GA_Warrior_Attack, BP_Player 능력, BP_Dummy")
