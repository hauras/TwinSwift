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
GOLEM_SLAM_FX = "/Game/Assets/VFX/Boss/NS_GolemSlam"

# 골렘 스탯 (시작할 때 한 번 덮어씀). 조정은 여기만
# 체력: 지금은 테스트용으로 낮게 (영상용 2~3분 전투는 약 3000). 공격력 15 → 내려치기 37 · 손등 22 · 점프 45
GOLEM_STATS = {"MaxHealth": 500.0, "Health": 500.0, "AttackPower": 15.0}
STATS_SOURCE = "/Game/Abilities/Warrior/GE_Warrior_DefaultAttributes"
STATS_GE = "/Game/Abilities/Boss/GE_Golem_DefaultAttributes"

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
            # 골렘 땅 충격 (VFX/make_golem_slam.py). 처음 쓴 NS_Dark_Stone_Impact는 보라 연기·번개라 골렘과 안 어울려 교체
            "impact_effect": unreal.load_asset(GOLEM_SLAM_FX),
            "impact_effect_scale": 1.1,
            "hit_stun_duration": 0.0,
        },
    },
    {
        # 손등 치기 1번: 오른손이 몸 왼쪽 → 오른쪽으로 앞을 가로지르는 1.05초 (앞 약 2.5m, 손 위치 실측)
        # 1.3~1.5초 왼손·1.8~2.1초 오른손이 앞으로 나오는 건 몸이 쏠렸다 돌아오는 마무리 (타격 아님)
        # 처음의 Mutant Punch(허리 높이 짧은 훅)는 쿼터뷰에서 잘 안 읽혀 교체
        "ga": "GA_Golem_Punch",
        "montage": "AM_Golem_Punch",
        "anim": "A_Golem_Backhand",
        "tag": "Ability.Boss.Golem.Punch",
        "cooldown_tag": "Cooldown.Boss.Golem.Punch",
        "props": {
            # 조금 빠르게: 실제 예고 0.91초 (내려치기 1.35초보다 짧음)
            "montage_play_rate": 1.15,
            "hit_times": [1.05],
            "shape": unreal.BossAttackShape.FAN,
            "forward_offset": 0.0,
            "radius": 320.0,
            # 가로로 쓸어서 넓게
            "fan_angle": 120.0,
            "damage_multiplier": 1.5,
            "cooldown_duration": 3.0,
            # 허공을 치는 공격이라 바닥 이펙트 없음. 흔들림은 가까울 때만
            "impact_effect": None,
            "impact_shake_outer_radius": 800.0,
            "hit_stun_duration": 0.0,
        },
    },
    {
        # 점프 내려찍기: 멀리 있는 플레이어에게 뛰어듦 (원본 A_Golem_JumpAttack 골반 실측, 메시 단위 ×2.7)
        #   0~0.47초 웅크림 → 0.47~1.6초 도약(골반 앞으로 약 2.6m, 최고 높이 1.0초) → 1.67초 두 손이 바닥 (몸 앞 약 1.75m)
        # 애니는 제자리 버전(make_golem_in_place.py), 앞으로 가는 건 능력이 캡슐을 옮김
        "ga": "GA_Golem_JumpSlam",
        "montage": "AM_Golem_JumpSlam",
        "anim": "A_Golem_JumpAttack_InPlace",
        "tag": "Ability.Boss.Golem.JumpSlam",
        "cooldown_tag": "Cooldown.Boss.Golem.JumpSlam",
        "props": {
            "montage_play_rate": 1.0,
            "hit_times": [1.67],
            "shape": unreal.BossAttackShape.CIRCLE,
            # 손이 떨어지는 곳 = 착지 지점 앞 175
            "forward_offset": 175.0,
            "radius": 300.0,
            "damage_multiplier": 3.0,
            "cooldown_duration": 8.0,
            # bool 속성은 파이썬에서 앞의 b가 빠짐 (bLeapToTarget → leap_to_target)
            "leap_to_target": True,
            "leap_start_time": 0.47,
            "leap_end_time": 1.6,
            "max_leap_distance": 800.0,
            # 가장 큰 공격이라 이펙트도 가장 크게
            "impact_effect": unreal.load_asset(GOLEM_SLAM_FX),
            "impact_effect_scale": 1.8,
            "impact_shake_outer_radius": 2500.0,
            # 맞으면 1초 경직 (이동·공격·회피 불가) → 교체로 탈출
            "hit_stun_duration": 1.0,
        },
    },
    {
        # 등장: 점프 내려찍기와 같은 모션으로 경기장 가장자리에서 가운데로 (노릴 위치는 ASBossEncounter가 보스에게 정해 줌)
        # 피해·예고 없음, 이펙트·흔들림은 가장 크게. 패턴 선택기 목록(PATTERNS)에는 넣지 않음
        "ga": "GA_Golem_Intro",
        "montage": "AM_Golem_JumpSlam",
        "anim": "A_Golem_JumpAttack_InPlace",
        "tag": "Ability.Boss.Golem.Intro",
        "cooldown_tag": None,
        "props": {
            "montage_play_rate": 1.0,
            "hit_times": [1.67],
            "shape": unreal.BossAttackShape.CIRCLE,
            "forward_offset": 175.0,
            "radius": 300.0,
            "deal_damage": False,
            "cooldown_duration": 0.0,
            "leap_to_target": True,
            "leap_start_time": 0.47,
            "leap_end_time": 1.6,
            # 가장자리(기둥 위)에서 가운데까지 (경기장 반경보다 넉넉히)
            "max_leap_distance": 3000.0,
            # 기둥 위에서 뛰어오르는 느낌: 직선(기둥 꼭대기 → 바닥) 가운데에서 2.5m 위로
            "leap_arc_height": 250.0,
            "impact_effect": unreal.load_asset(GOLEM_SLAM_FX),
            "impact_effect_scale": 2.5,
            "impact_shake_outer_radius": 5000.0,
            "hit_stun_duration": 0.0,
            "telegraph_class": None,
        },
    },
]


def tag_container(*names):
    c = unreal.GameplayTagContainer()
    inner = ",".join(f'(TagName="{n}")' for n in names)
    c.import_text(f"(GameplayTags=({inner}))")
    return c


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text(f'(TagName="{name}")')
    return tag


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
    cdo.set_editor_property("cooldown_tags", tag_container(atk["cooldown_tag"]) if atk["cooldown_tag"] else unreal.GameplayTagContainer())

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
boss_cdo.set_editor_property("intro_ability_tag", make_tag("Ability.Boss.Golem.Intro"))

# 스탯 GE: 플레이어 것(즉시 적용, 덮어쓰기 3개)을 복사해 값만 바꿈
if not eal.does_asset_exist(STATS_GE):
    eal.duplicate_asset(STATS_SOURCE, STATS_GE)
stats_bp = unreal.load_asset(STATS_GE)
stats_cdo = unreal.get_default_object(stats_bp.generated_class())
modifiers = list(stats_cdo.get_editor_property("modifiers"))
for modifier in modifiers:
    text = modifier.export_text()
    attribute = text.split('AttributeName="')[1].split('"')[0]
    if attribute in GOLEM_STATS:
        # ScalableFloatMagnitude의 Value만 바꿈 (첫 번째 Value=가 그 값)
        head, tail = text.split("ScalableFloatMagnitude=(Value=", 1)
        old_value, rest = tail.split(",", 1)
        modifier.import_text(f"{head}ScalableFloatMagnitude=(Value={GOLEM_STATS[attribute]:.6f},{rest}")
stats_cdo.set_editor_property("modifiers", modifiers)
unreal.BlueprintEditorLibrary.compile_blueprint(stats_bp)
eal.save_loaded_asset(stats_bp)
boss_cdo.set_editor_property("default_attributes_effect", stats_bp.generated_class())
print("스탯:", [m.export_text().split('AttributeName="')[1].split('"')[0] + "=" + m.export_text().split("ScalableFloatMagnitude=(Value=")[1].split(",")[0]
               for m in stats_cdo.get_editor_property("modifiers")])

# 사망 몽타주: 끝나도 쓰러진 자세로 남게 자동 블렌드 아웃 끔 (켜 두면 끝나는 순간 Idle로 일어남)
death = get_or_create_montage("AM_Golem_Death", "A_Golem_Death")
death.set_editor_property("enable_auto_blend_out", False)
eal.save_loaded_asset(death)
boss_cdo.set_editor_property("death_montage", death)

# 패턴 선택기 목록 (거리 cm). 판정 범위에서 정함:
#   내려치기: 앞 280 중심 · 반경 220 원 → 앞 60~500이 맞음. 너무 붙으면 원 뒤쪽이라 빗나가서 최소 150
#   손등 치기: 반경 320 부채꼴 → 0~320
PATTERNS = [
    {"tag": "Ability.Boss.Golem.Slam", "min": 150.0, "max": 480.0, "preferred": 320.0, "weight": 1.0},
    {"tag": "Ability.Boss.Golem.Punch", "min": 0.0, "max": 320.0, "preferred": 180.0, "weight": 1.0},
    # 점프: 최대 이동 800 + 손 위치 175 → 975까지 닿지만, 너무 멀면 예고를 보고 쉽게 피하므로 900까지. 400~480은 내려치기와 겹침
    {"tag": "Ability.Boss.Golem.JumpSlam", "min": 400.0, "max": 900.0, "preferred": 650.0, "weight": 1.0},
]


def make_pattern(p):
    # USTRUCT는 키워드 인자로 못 만들어서 만든 뒤 값을 넣음
    pattern = unreal.SBossPattern()
    pattern.set_editor_property("ability_tag", make_tag(p["tag"]))
    pattern.set_editor_property("min_range", p["min"])
    pattern.set_editor_property("max_range", p["max"])
    pattern.set_editor_property("preferred_range", p["preferred"])
    pattern.set_editor_property("weight", p["weight"])
    return pattern


selector = boss_cdo.get_editor_property("pattern_component")
selector.set_editor_property("patterns", [make_pattern(p) for p in PATTERNS])
print("패턴:", [(str(p.get_editor_property("ability_tag").get_editor_property("tag_name")), p.get_editor_property("min_range"), p.get_editor_property("max_range"))
                for p in selector.get_editor_property("patterns")])
unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
eal.save_loaded_asset(boss_bp)
print("BP_Boss_Golem StartupAbilities:", [c.get_name() for c in boss_cdo.get_editor_property("startup_abilities")])
