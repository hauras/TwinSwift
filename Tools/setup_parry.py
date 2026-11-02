# 전사 반격 자세 (우클릭): 몽타주 2개 + GA_Warrior_Parry + 플레이어 전사·거울 보스 능력 목록
# 입력(IA_Warrior_Parry 우클릭)은 setup_ability_input.py
# 자세: 믹사모 막기(Great Sword Blocking)를 기사에 리타깃한 A_Knight_Block (retarget_knight_block.py → fix_knight_block_root.py)
#   처음에는 기사 팩에 막기 동작이 없어 A_Knight_Casting의 칼을 세운 부분(2.2초~)을 썼음
# 반격: 안 쓰던 회전 베기 A_Knight_SpinAttack
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

ANIM_DIR = "/Game/Assets/External/Characters/Knight_Free/Animations"
MONTAGE_DIR = "/Game/Assets/Characters/Warrior/Montages"
GA_DIR = "/Game/Abilities/Warrior"
GA_NAME = "GA_Warrior_Parry"


def montage(name, anim_name):
    path = f"{MONTAGE_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", unreal.load_asset(f"{ANIM_DIR}/{anim_name}"))
    m = tools.create_asset(name, MONTAGE_DIR, unreal.AnimMontage, factory)
    eal.save_loaded_asset(m)
    return m


# 예전 자세 몽타주(Casting으로 만든 것)가 남아 있으면 지우고 막기 애니로 다시
STANCE_ANIM = "A_Knight_Block"
stance_path = f"{MONTAGE_DIR}/AM_Warrior_ParryStance"
if eal.does_asset_exist(stance_path):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True)
    uses = [str(d) for d in registry.get_dependencies(stance_path, options) or []]
    if f"{ANIM_DIR}/{STANCE_ANIM}" not in uses:
        eal.delete_asset(stance_path)
        print("예전 자세 몽타주 지움 (원본 애니:", [u for u in uses if "Animations" in u], ")")
stance = montage("AM_Warrior_ParryStance", STANCE_ANIM)
counter = montage("AM_Warrior_ParryCounter", "A_Knight_SpinAttack")

# 칼이 지나가는 순간 신호: 손이 가장 빠른 때(약 1.12초, AnimPose로 잼) → 이때 피해·보스 경직. 다시 실행해도 겹치지 않게 트랙을 비우고 다시 깜
COUNTER_HIT_TIME = 1.12
HIT_TRACK = "Hit"
anim_lib = unreal.AnimationLibrary
if HIT_TRACK not in [str(n) for n in anim_lib.get_animation_notify_track_names(counter)]:
    anim_lib.add_animation_notify_track(counter, HIT_TRACK)
anim_lib.remove_animation_notify_events_by_track(counter, HIT_TRACK)
hit_notify = anim_lib.add_animation_notify_event(counter, HIT_TRACK, COUNTER_HIT_TIME, unreal.AN_SendEvent)
hit_tag = unreal.GameplayTag()
hit_tag.import_text('(TagName="Event.Montage.Hit")')
hit_notify.set_editor_property("event_tag", hit_tag)
eal.save_loaded_asset(counter, only_if_is_dirty=False)

# ---------- 능력 BP ----------
ga_path = f"{GA_DIR}/{GA_NAME}"
if eal.does_asset_exist(ga_path):
    ga_bp = unreal.load_asset(ga_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.SGA_Parry)
    ga_bp = tools.create_asset(GA_NAME, GA_DIR, unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
ga = unreal.get_default_object(ga_bp.generated_class())
ga.set_editor_property("stance_montage", stance)
ga.set_editor_property("stance_start_time", 0.0)
ga.set_editor_property("counter_montage", counter)

# 막은 순간 연출 (칼 파동이 부딪히는 이펙트, 골렘 갑옷 파괴와 같은 흔들림)
ga.set_editor_property("parry_effect", unreal.load_asset("/Game/Assets/VFX/Common/Assist/NS_SwordWave_Hit"))
ga.set_editor_property("parry_shake", unreal.load_class(None, "/Script/TwinSwift.SCameraShake_BossImpact"))

# 다른 전사 능력과 같은 피해 GE
slam = unreal.get_default_object(unreal.load_asset(f"{GA_DIR}/GA_Warrior_GroundSlam").generated_class())
ga.set_editor_property("damage_effect_class", slam.get_editor_property("damage_effect_class"))
unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
eal.save_loaded_asset(ga_bp)
parry_class = ga_bp.generated_class()


def add_ability(abilities):
    abilities = list(abilities)
    if parry_class not in abilities:
        abilities.append(parry_class)
    return abilities


# ---------- 플레이어 전사 ----------
player_bp = unreal.load_asset("/Game/Blueprints/Characters/BP_Player")
player = unreal.get_default_object(player_bp.generated_class())
config = player.get_editor_property("warrior_config")
config.set_editor_property("startup_abilities", add_ability(config.get_editor_property("startup_abilities")))
player.set_editor_property("warrior_config", config)
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp)

# ---------- 거울 보스 (내 반격을 따라 함) ----------
mirror_bp = unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Mirror")
mirror = unreal.get_default_object(mirror_bp.generated_class())
mirror.set_editor_property("startup_abilities", add_ability(mirror.get_editor_property("startup_abilities")))
unreal.BlueprintEditorLibrary.compile_blueprint(mirror_bp)
eal.save_loaded_asset(mirror_bp)

player = unreal.get_default_object(player_bp.generated_class())
mirror = unreal.get_default_object(mirror_bp.generated_class())
print("GA:", ga_bp.get_path_name(), "/ 자세", stance.get_name(), "/ 반격", counter.get_name(), "/ 피해 GE", ga.get_editor_property("damage_effect_class"))
print("전사 능력:", [a.get_name() for a in player.get_editor_property("warrior_config").get_editor_property("startup_abilities")])
print("거울 능력:", [a.get_name() for a in mirror.get_editor_property("startup_abilities")])
print("PARRY_SETUP_COMPLETE")
