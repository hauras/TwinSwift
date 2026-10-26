# 밀려남: 색을 못 맞춘 충격파에 맞으면 뒤로 밀림
# 1) GA_Knockback (부모 SGA_Knockback) 생성
# 2) BP_Player 전사·마법사 슬롯 시작 능력에 추가 (ASC가 캐릭터마다 따로라 둘 다)
# 3) DA_Conductor_Shockwave: 48발 (탄 사이 틈이 캐릭터 폭보다 좁게), 밀기 600cm
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡아 에디터 파이썬이 붙잡지 않게
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    ga_path = "/Game/Abilities/Common/GA_Knockback"

    # 1) 능력 BP
    if eal.does_asset_exist(ga_path):
        ga_bp = unreal.load_asset(ga_path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SGA_Knockback"))
        ga_bp = tools.create_asset("GA_Knockback", "/Game/Abilities/Common", unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    eal.save_loaded_asset(ga_bp)
    ga_class = ga_bp.generated_class()

    # 2) 두 슬롯에 추가 (이미 있으면 그대로)
    player_bp = unreal.load_asset("/Game/Blueprints/Characters/BP_Player")
    player = unreal.get_default_object(player_bp.generated_class())
    for name in ("warrior_config", "mage_config"):
        config = player.get_editor_property(name)
        abilities = list(config.get_editor_property("startup_abilities"))
        if ga_class not in abilities:
            abilities.append(ga_class)
        config.set_editor_property("startup_abilities", abilities)
        player.set_editor_property(name, config)
        print(name, [a.get_name() for a in abilities])
    unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
    eal.save_loaded_asset(player_bp)

    # 3) 충격파 데이터
    da = unreal.load_asset("/Game/Abilities/Boss/Patterns/DA_Conductor_Shockwave")
    events = list(da.get_editor_property("events"))
    for event in events:
        event.set_editor_property("bullet_count", 48)
        event.set_editor_property("knockback_strength", 600.0)
    da.set_editor_property("events", events)
    eal.save_loaded_asset(da, only_if_is_dirty=False)
    print("충격파:", [(e.get_editor_property("bullet_count"), e.get_editor_property("knockback_strength")) for e in events])

    print("KNOCKBACK_COMPLETE")


run()
