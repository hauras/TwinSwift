# 지휘자 협공 벽 (2페이즈): 플레이어 양옆 10m에서 벽이 생겨 조여 옴 (1.25초 뒤 닿음)
# 차례 3번 (1.2초 간격 = 회피 쿨다운 1초보다 김)
#   1) 0.5초 양쪽 주황  → 전사로 막음 (색 판단)
#   2) 1.7초 주황 + 파랑 → 한 캐릭터로 다 못 막음 → 닿는 순간 회피·교체 무적 (타이밍)
#   3) 2.9초 양쪽 파랑  → 마법사로 막음 (2번을 교체 무적으로 넘겼으면 이미 마법사)
# 벽 48발·2400cm(약 51cm 간격, 1.25초 안에 옆으로 돌아 나가기 어렵게), 속도 800, 수명 2.5초(지나간 뒤 경기장을 쓸지 않게)
# 1) AM_Conductor_Pincer (A_Conductor_2HMagicAttack_05, 3.53초)
# 2) DA_Conductor_Pincer  3) GA_Conductor_Pincer (쿨다운 9초)
# 4) BP_Boss_Conductor: 시작 능력 + 패턴 선택기 (2페이즈부터, 거리 0~6000, 선호 1000, 가중치 1.5)
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    base = "/Game/Assets/Characters/Boss/Conductor"
    anim_path = f"{base}/Animations/A_Conductor_2HMagicAttack_05"
    montage_path = f"{base}/Montages/AM_Conductor_Pincer"
    da_path = "/Game/Abilities/Boss/Patterns/DA_Conductor_Pincer"
    ga_path = "/Game/Abilities/Boss/GA_Conductor_Pincer"
    boss_path = "/Game/Blueprints/Boss/BP_Boss_Conductor"
    ability_tag = "Ability.Boss.Conductor.Pincer"

    def tags(*names):
        c = unreal.GameplayTagContainer()
        c.import_text("(GameplayTags=(" + ",".join(f'(TagName="{n}")' for n in names) + "))")
        return c

    def tag(name):
        t = unreal.GameplayTag()
        t.import_text(f'(TagName="{name}")')
        return t

    # 1) 몽타주
    if eal.does_asset_exist(montage_path):
        montage = unreal.load_asset(montage_path)
    else:
        factory = unreal.AnimMontageFactory()
        factory.set_editor_property("source_animation", unreal.load_asset(anim_path))
        montage = tools.create_asset("AM_Conductor_Pincer", f"{base}/Montages", unreal.AnimMontage, factory)
    eal.save_loaded_asset(montage)

    # 2) 패턴 데이터: 차례마다 왼쪽(90)·오른쪽(-90) 두 줄
    if eal.does_asset_exist(da_path):
        da = unreal.load_asset(da_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.SBulletPatternData)
        da = tools.create_asset("DA_Conductor_Pincer", "/Game/Abilities/Boss/Patterns", unreal.SBulletPatternData, factory)

    orange = unreal.BulletColor.ORANGE
    blue = unreal.BulletColor.BLUE

    def wall(fire_time, angle, color):
        row = unreal.SBulletPatternEvent()
        for k, v in dict(fire_time=fire_time, bullet_count=48, shape=unreal.BulletShape.WALL, wall_width=2400.0,
                         speed=800.0, angle_offset=angle, color=color, repeat_count=1, damage_multiplier=1.0,
                         life_time=2.5, spawn_around_target=True, spawn_distance=1000.0).items():
            row.set_editor_property(k, v)
        return row

    waves = [(0.5, orange, orange), (1.7, orange, blue), (2.9, blue, blue)]
    events = []
    for fire_time, left, right in waves:
        events.append(wall(fire_time, 90.0, left))
        events.append(wall(fire_time, -90.0, right))
    da.set_editor_property("events", events)
    eal.save_loaded_asset(da, only_if_is_dirty=False)

    # 3) 능력 BP
    if eal.does_asset_exist(ga_path):
        ga_bp = unreal.load_asset(ga_path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SGA_ConductorPattern"))
        ga_bp = tools.create_asset("GA_Conductor_Pincer", "/Game/Abilities/Boss", unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    ga = unreal.get_default_object(ga_bp.generated_class())
    ga.set_editor_property("cast_montage", montage)
    ga.set_editor_property("pattern_data", da)
    ga.set_editor_property("cooldown_duration", 9.0)
    ga.set_editor_property("cooldown_tags", tags("Cooldown.Boss.Conductor.Pincer"))
    ga.set_editor_property("ability_tags", tags(ability_tag))
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    eal.save_loaded_asset(ga_bp)

    # 4) 보스: 시작 능력 + 패턴 목록 (같은 태그가 있으면 바꿔 넣음)
    boss_bp = unreal.load_asset(boss_path)
    boss = unreal.get_default_object(boss_bp.generated_class())
    abilities = list(boss.get_editor_property("startup_abilities"))
    if ga_bp.generated_class() not in abilities:
        abilities.append(ga_bp.generated_class())
    boss.set_editor_property("startup_abilities", abilities)

    selector = boss.get_editor_property("pattern_component")
    patterns = [p for p in selector.get_editor_property("patterns")
                if str(p.get_editor_property("ability_tag").get_editor_property("tag_name")) != ability_tag]
    entry = unreal.SBossPattern()
    for k, v in dict(ability_tag=tag(ability_tag), min_range=0.0, max_range=6000.0, preferred_range=1000.0,
                     weight=1.5, min_phase=2, max_phase=0).items():
        entry.set_editor_property(k, v)
    patterns.append(entry)
    selector.set_editor_property("patterns", patterns)
    unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
    eal.save_loaded_asset(boss_bp)

    print("협공 벽: 몽타주", round(montage.get_play_length(), 2), "초, 지시", len(events), "줄",
          "/ 패턴:", [(str(p.get_editor_property("ability_tag").get_editor_property("tag_name")).split(".")[-1],
                      p.get_editor_property("min_phase"), p.get_editor_property("max_phase")) for p in patterns])
    print("CONDUCTOR_PINCER_COMPLETE")


run()
