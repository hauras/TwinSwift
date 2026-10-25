# 단계 6-2 회피 에셋: 구르기 몽타주 + 회피 능력 블루프린트 2개 + 시작 능력에 추가
#   GA_Warrior_Roll (부모 USGA_Dodge): 구르기 몽타주, 0.4초 동안 450cm 밀기
#   GA_Mage_Blink   (부모 USGA_Dodge): 몽타주 없음, 500cm 순간이동 (이동 시간 0)
# 조건: USGA_Dodge가 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

ROLL_ANIM = "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Roll"
ROLL_MONTAGE = "/Game/Assets/Characters/Warrior/Montages/AM_Warrior_Roll"

# (블루프린트 경로, 설정값)
DODGES = {
    "/Game/Abilities/Warrior/GA_Warrior_Roll": dict(
        dodge_distance=450.0, dodge_duration=0.4, invincible_time=0.3,
        montage=ROLL_MONTAGE, montage_play_rate=1.5, montage_start_time=0.5),
    "/Game/Abilities/Mage/GA_Mage_Blink": dict(
        dodge_distance=500.0, dodge_duration=0.0, invincible_time=0.3,
        montage=None, montage_play_rate=1.0, montage_start_time=0.0),
}


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 구르기 몽타주 ----------
if eal.does_asset_exist(ROLL_MONTAGE):
    montage = load(ROLL_MONTAGE)
else:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", load(ROLL_ANIM))
    folder, name = ROLL_MONTAGE.rsplit("/", 1)
    montage = tools.create_asset(name, folder, unreal.AnimMontage, factory)
for prop, value in (("blend_in", 0.08), ("blend_out", 0.15)):
    blend = montage.get_editor_property(prop)
    blend.set_editor_property("blend_time", value)
    montage.set_editor_property(prop, blend)
eal.save_loaded_asset(montage, only_if_is_dirty=False)

# ---------- 회피 능력 블루프린트 ----------
abilities = {}
for path, cfg in DODGES.items():
    folder, name = path.rsplit("/", 1)
    if eal.does_asset_exist(path):
        bp = load(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.SGA_Dodge)
        bp = tools.create_asset(name, folder, unreal.Blueprint, factory)
    cdo = unreal.get_default_object(bp.generated_class())
    for key in ("dodge_distance", "dodge_duration", "invincible_time", "montage_play_rate", "montage_start_time"):
        cdo.set_editor_property(key, cfg[key])
    cdo.set_editor_property("dodge_montage", load(cfg["montage"]) if cfg["montage"] else None)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    abilities[name] = bp.generated_class()
    print(name, {k: cdo.get_editor_property(k) for k in ("dodge_distance", "dodge_duration", "invincible_time", "montage_play_rate", "montage_start_time")},
          "몽타주", cdo.get_editor_property("dodge_montage"))

# ---------- 시작 능력에 추가 (이미 있으면 그대로) ----------
player_bp = load("/Game/Blueprints/Characters/BP_Player")
player = unreal.get_default_object(player_bp.generated_class())
for prop, ability in (("warrior_config", abilities["GA_Warrior_Roll"]), ("mage_config", abilities["GA_Mage_Blink"])):
    config = player.get_editor_property(prop)
    text = config.export_text()
    current = [s for s in text.split("StartupAbilities=(")[1].split("),DefaultAttributesEffect")[0].split(",") if s] if "StartupAbilities=(" in text else []
    path = f'"{ability.get_path_name()}"'
    if not any(ability.get_path_name() in c for c in current):
        current.append(path)
    effect = text.split("DefaultAttributesEffect=")[1].rstrip(")") if "DefaultAttributesEffect=" in text else "None"
    new = unreal.TwinSlotConfig()
    # 구조체 칸이 EditDefaultsOnly라 텍스트로 한 번에 넣음
    new.import_text(f"(StartupAbilities=({','.join(current)}),DefaultAttributesEffect={effect})")
    player.set_editor_property(prop, new)
    print(prop, player.get_editor_property(prop).export_text())
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
