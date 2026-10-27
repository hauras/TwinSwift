# 강화: GE 3개 + 강화 데이터 4개
# GE는 무한 지속(런 끝까지, 레벨을 다시 불러오면 사라짐), 수정자 하나 (MultiplyCompound: 같은 강화를 두 번 고르면 차례로 곱함 1.2 × 1.2)
# 전사·마법사 강화는 같은 GE(StrongAttack). 누구에게 붙일지는 데이터의 Target
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import unreal

GE_DIR = "/Game/Abilities/Upgrade"
DA_DIR = "/Game/Data/Upgrades"

EFFECTS = {
    "GE_Upgrade_AttackUp": ("AttackPower", 1.2),
    "GE_Upgrade_MaxHealthUp": ("MaxHealth", 1.25),
    "GE_Upgrade_StrongAttack": ("AttackPower", 1.35),
}

# 에셋 이름: (카드 제목, 설명, 대상, GE)
UPGRADES = {
    "DA_Upgrade_AttackUp": ("합주", "두 캐릭터 공격력 +20%", "BOTH", "GE_Upgrade_AttackUp"),
    "DA_Upgrade_MaxHealthUp": ("단단한 몸", "두 캐릭터 최대 체력 +25%", "BOTH", "GE_Upgrade_MaxHealthUp"),
    "DA_Upgrade_WarriorAttack": ("날 선 칼", "전사 공격력 +35%", "WARRIOR", "GE_Upgrade_StrongAttack"),
    "DA_Upgrade_MageAttack": ("깊은 마력", "마법사 공격력 +35%", "MAGE", "GE_Upgrade_StrongAttack"),
}


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()

    # GE
    effect_classes = {}
    for name, (attribute, value) in EFFECTS.items():
        path = f"{GE_DIR}/{name}"
        if eal.does_asset_exist(path):
            bp = unreal.load_asset(path)
        else:
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", unreal.GameplayEffect)
            bp = tools.create_asset(name, GE_DIR, unreal.Blueprint, factory)
        cdo = unreal.get_default_object(bp.generated_class())
        cdo.set_editor_property("duration_policy", unreal.GameplayEffectDurationType.INFINITE)
        modifier = unreal.GameplayModifierInfo()
        modifier.import_text(
            f'(Attribute=(AttributeName="{attribute}",Attribute=/Script/TwinSwift.SAttributeSet:{attribute},AttributeOwner=/Script/TwinSwift.SAttributeSet),'
            'ModifierOp=MultiplyCompound,'
            f'ModifierMagnitude=(MagnitudeCalculationType=ScalableFloat,ScalableFloatMagnitude=(Value={value})))')
        cdo.set_editor_property("modifiers", [modifier])
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        eal.save_loaded_asset(bp, only_if_is_dirty=False)
        effect_classes[name] = bp.generated_class()
        print(name, cdo.get_editor_property("modifiers")[0].export_text()[:160])

    # 강화 데이터
    for name, (title, description, target, effect) in UPGRADES.items():
        path = f"{DA_DIR}/{name}"
        if eal.does_asset_exist(path):
            da = unreal.load_asset(path)
        else:
            factory = unreal.DataAssetFactory()
            factory.set_editor_property("data_asset_class", unreal.SUpgradeData)
            da = tools.create_asset(name, DA_DIR, unreal.SUpgradeData, factory)
        da.set_editor_property("display_name", unreal.Text(title))
        da.set_editor_property("description", unreal.Text(description))
        da.set_editor_property("target", getattr(unreal.UpgradeTarget, target))
        da.set_editor_property("effect", effect_classes[effect])
        eal.save_loaded_asset(da, only_if_is_dirty=False)
        print(name, da.get_editor_property("display_name"), da.get_editor_property("target"), da.get_editor_property("effect").get_name())

    print("UPGRADES_COMPLETE")


run()


# BP_GameMode에 후보 4개 + 카드 화면 연결 (게임 모드에 UpgradePool이 빌드돼 있어야 함)
def connect():
    gm_bp = unreal.load_asset("/Game/Blueprints/Core/BP_GameMode")
    gm = unreal.get_default_object(gm_bp.generated_class())
    pool = [unreal.load_asset(f"{DA_DIR}/{name}") for name in UPGRADES]
    gm.set_editor_property("upgrade_pool", pool)
    gm.set_editor_property("upgrade_select_class", unreal.load_asset("/Game/UI/WBP_UpgradeSelect").generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(gm_bp)
    print("BP_GameMode 후보:", [d.get_name() for d in gm.get_editor_property("upgrade_pool")],
          "/ 화면:", gm.get_editor_property("upgrade_select_class").get_name())


connect()
