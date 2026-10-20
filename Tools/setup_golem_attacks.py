# 골렘 공격 에셋: 몽타주 + GA(USGA_BossAttack의 BP) 만들고 BP_Boss_Golem의 StartupAbilities에 추가
# 맞는 시간은 손 위치를 프레임별로 재서 정함 (A_Golem_Swipe: 왼손이 머리 위 1.23초 → 바닥 1.40초, 앞 약 3.2m)
# 다시 실행해도 됨: 있는 에셋은 값만 다시 넣음
import unreal

if unreal.EditorLevelLibrary.get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

ANIM_DIR = "/Game/Assets/Characters/Boss/Golem/Animations"
MONTAGE_DIR = "/Game/Assets/Characters/Boss/Golem/Montages"
GA_DIR = "/Game/Abilities/Boss"
BOSS_BP = "/Game/Blueprints/Enemy/BP_Boss_Golem"
PARENT = unreal.load_class(None, "/Script/TwinSwift.SGA_BossAttack")

ATTACKS = [
    {
        "ga": "GA_Golem_Slam",
        "montage": "AM_Golem_Slam",
        "anim": "A_Golem_Swipe",
        "tag": "Ability.Boss.Golem.Slam",
        "cooldown_tag": "Cooldown.Boss.Golem.Slam",
        "props": {
            "montage_play_rate": 1.0,
            "hit_times": [1.35],
            "shape": unreal.BossAttackShape.CIRCLE,
            # 손이 닿는 곳(앞 약 3.2m)이 원의 가운데쯤 오게
            "forward_offset": 280.0,
            "radius": 220.0,
            "damage_multiplier": 2.5,
            "cooldown_duration": 4.0,
            # 바위 파편 (균열 폭발 NS_CrackBurst의 원본. 그쪽은 주황색으로 바꿔서 겹치지 않음)
            "impact_effect": unreal.load_asset("/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Stone_Impact"),
            "impact_effect_scale": 1.5,
        },
    },
]


def tag_container(*names):
    c = unreal.GameplayTagContainer()
    inner = ",".join(f'(TagName="{n}")' for n in names)
    c.import_text(f"(GameplayTags=({inner}))")
    return c


def get_or_create_montage(name, anim_name):
    path = f"{MONTAGE_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", unreal.load_asset(f"{ANIM_DIR}/{anim_name}"))
    montage = tools.create_asset(name, MONTAGE_DIR, unreal.AnimMontage, factory)
    eal.save_loaded_asset(montage)
    return montage


def get_or_create_ga(name):
    path = f"{GA_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", PARENT)
    return tools.create_asset(name, GA_DIR, unreal.Blueprint, factory)


ga_classes = []
for atk in ATTACKS:
    montage = get_or_create_montage(atk["montage"], atk["anim"])
    bp = get_or_create_ga(atk["ga"])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())

    cdo.set_editor_property("attack_montage", montage)
    for key, value in atk["props"].items():
        cdo.set_editor_property(key, value)
    cdo.set_editor_property("cooldown_tags", tag_container(atk["cooldown_tag"]))

    # 능력 이름표: 5.5부터 AbilityTags → AssetTags로 이름이 바뀜
    for prop in ("asset_tags", "ability_tags"):
        try:
            cdo.set_editor_property(prop, tag_container(atk["tag"]))
            print(atk["ga"], "태그 속성:", prop)
            break
        except Exception:
            continue

    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    ga_classes.append(bp.generated_class())

    print(atk["ga"], "몽타주", montage.get_name(), round(montage.get_play_length(), 2),
          "맞는 시간", list(cdo.get_editor_property("hit_times")),
          "모양", cdo.get_editor_property("shape"),
          "앞", cdo.get_editor_property("forward_offset"),
          "반경", cdo.get_editor_property("radius"))

# 보스 BP에 능력 추가 (이미 있으면 그대로)
boss_bp = unreal.load_asset(BOSS_BP)
boss_cdo = unreal.get_default_object(boss_bp.generated_class())
abilities = list(boss_cdo.get_editor_property("startup_abilities"))
for cls in ga_classes:
    if cls not in abilities:
        abilities.append(cls)
boss_cdo.set_editor_property("startup_abilities", abilities)
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
eal.save_loaded_asset(boss_bp)
print("BP_Boss_Golem StartupAbilities:", [c.get_name() for c in boss_cdo.get_editor_property("startup_abilities")])
