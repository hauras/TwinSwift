# 단계 3 에셋: 능력 BP, BP_Player에 능력 지급, 허수아비 BP + 맵 배치 (몽타주·콤보는 setup_combo.py)
# 조건: GA_AttackBase·ANS_WeaponTrace·EnemyCharacter가 들어간 C++이 빌드돼 있어야 함
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
anim_lib = unreal.AnimationLibrary


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 1. 몽타주·콤보 단계는 setup_combo.py에서 ----------

# ---------- 2. 능력 BP: GA_Warrior_Attack (부모 C++ GA_AttackBase) ----------
ga_path = "/Game/Abilities/Warrior/GA_Warrior_Attack"
if eal.does_asset_exist(ga_path):
    ga_bp = load(ga_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.GA_AttackBase)
    ga_bp = tools.create_asset("GA_Warrior_Attack", "/Game/Abilities/Warrior", unreal.Blueprint, factory)
    print("만듦:", ga_path)
unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
eal.save_loaded_asset(ga_bp, only_if_is_dirty=False)

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

print("완료: GA_Warrior_Attack, BP_Player 능력, BP_Dummy")
