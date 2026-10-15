# 캐릭터별 시작 스탯 GE 2개를 만들고 BP_Player의 WarriorConfig·MageConfig에 연결
# GE: Instant, MaxHealth → Health → AttackPower 순서로 Override
#   (MaxHealth를 먼저: 체력은 최대 체력을 넘지 못하게 막아 둬서, Health가 먼저면 예전 최대 체력으로 잘림)
# 숫자를 바꾸려면 STATS만 고치고 다시 실행 (에디터에서 GE를 직접 고쳐도 됨)
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# (GE 경로, 최대 체력, 공격력)
STATS = {
    "warrior": ("/Game/Abilities/Warrior/GE_Warrior_DefaultAttributes", 120.0, 10.0),
    "mage": ("/Game/Abilities/Mage/GE_Mage_DefaultAttributes", 90.0, 14.0),
}


def modifier(attribute, value):
    info = unreal.GameplayModifierInfo()
    info.import_text(
        f'(Attribute=(AttributeName="{attribute}",Attribute=/Script/TwinSwift.SAttributeSet:{attribute},AttributeOwner=/Script/TwinSwift.SAttributeSet),'
        f'ModifierOp=Override,'
        f'ModifierMagnitude=(MagnitudeCalculationType=ScalableFloat,ScalableFloatMagnitude=(Value={value})))')
    return info


def make_stats_ge(path, max_health, attack_power):
    folder, name = path.rsplit("/", 1)
    if eal.does_asset_exist(path):
        bp = unreal.load_asset(path + "." + name)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.GameplayEffect)
        bp = tools.create_asset(name, folder, unreal.Blueprint, factory)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("duration_policy", unreal.GameplayEffectDurationType.INSTANT)
    cdo.set_editor_property("modifiers", [
        modifier("MaxHealth", max_health),
        modifier("Health", max_health),
        modifier("AttackPower", attack_power),
    ])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    return bp


ge = {key: make_stats_ge(*value) for key, value in STATS.items()}

player_bp = unreal.load_asset("/Game/Blueprints/BP_Player.BP_Player")
cdo = unreal.get_default_object(player_bp.generated_class())
attack = unreal.load_asset("/Game/Abilities/Warrior/GA_Warrior_Attack.GA_Warrior_Attack")
configs = {
    "warrior_config": (ge["warrior"], [attack.generated_class()]),
    "mage_config": (ge["mage"], []),
}
for prop, (stats_bp, abilities) in configs.items():
    # 구조체 칸이 EditDefaultsOnly라 텍스트로 한 번에 넣음
    config = unreal.TwinSlotConfig()
    ability_text = ",".join(f'"{a.get_path_name()}"' for a in abilities)
    config.import_text(f'(StartupAbilities=({ability_text}),DefaultAttributesEffect="{stats_bp.generated_class().get_path_name()}")')
    cdo.set_editor_property(prop, config)
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)

for prop in configs:
    print(prop, cdo.get_editor_property(prop).export_text())
for key, bp in ge.items():
    mods = unreal.get_default_object(bp.generated_class()).get_editor_property("modifiers")
    print(key, [m.export_text().split("AttributeName=")[1].split(",")[0] + " " + m.export_text().split("Value=")[1].split(")")[0] for m in mods])
