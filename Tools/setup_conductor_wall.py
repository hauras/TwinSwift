# 지휘자 색 벽: 일렬로 선 탄 벽이 밀려옴, 그 색 캐릭터여야 통과 (원거리 패턴)
# 교체 쿨다운 2초 → 색을 0.6초마다 바꾸면 못 깸. 같은 색 2장(0.8초 간격) → 2.2초 뒤 다른 색 1장
# 1) AM_Conductor_Wall (A_Conductor_2HMagicAttack_03, 4.3초: 마지막 벽 3.6초가 블렌드 아웃 전)
# 2) DA_Conductor_Wall: 주황 벽 0.6초부터 2번(0.8초 간격), 파랑 벽 3.6초. 32발·1600cm(약 52cm 간격), 속도 700
# 3) GA_Conductor_Wall (부모 SGA_ConductorPattern), 쿨다운 8초
# 4) BP_Boss_Conductor: 시작 능력 + 패턴 선택기 (거리 400~6000, 선호 1200, 가중치 1)
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    base = "/Game/Assets/Characters/Boss/Conductor"
    anim_path = f"{base}/Animations/A_Conductor_2HMagicAttack_03"
    montage_path = f"{base}/Montages/AM_Conductor_Wall"
    da_path = "/Game/Abilities/Boss/Patterns/DA_Conductor_Wall"
    ga_path = "/Game/Abilities/Boss/GA_Conductor_Wall"
    boss_path = "/Game/Blueprints/Boss/BP_Boss_Conductor"
    ability_tag = "Ability.Boss.Conductor.Wall"

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
        montage = tools.create_asset("AM_Conductor_Wall", f"{base}/Montages", unreal.AnimMontage, factory)
    eal.save_loaded_asset(montage)

    # 2) 패턴 데이터
    if eal.does_asset_exist(da_path):
        da = unreal.load_asset(da_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.SBulletPatternData)
        da = tools.create_asset("DA_Conductor_Wall", "/Game/Abilities/Boss/Patterns", unreal.SBulletPatternData, factory)

    def wall(fire_time, color, repeat_count, repeat_interval):
        row = unreal.SBulletPatternEvent()
        for k, v in dict(fire_time=fire_time, bullet_count=32, shape=unreal.BulletShape.WALL, wall_width=1600.0,
                         speed=700.0, color=color, repeat_count=repeat_count, repeat_interval=repeat_interval,
                         damage_multiplier=1.0).items():
            row.set_editor_property(k, v)
        return row

    da.set_editor_property("events", [
        wall(0.6, unreal.BulletColor.ORANGE, 2, 0.8),
        wall(3.6, unreal.BulletColor.BLUE, 1, 0.15),
    ])
    eal.save_loaded_asset(da, only_if_is_dirty=False)

    # 3) 능력 BP
    if eal.does_asset_exist(ga_path):
        ga_bp = unreal.load_asset(ga_path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SGA_ConductorPattern"))
        ga_bp = tools.create_asset("GA_Conductor_Wall", "/Game/Abilities/Boss", unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    ga = unreal.get_default_object(ga_bp.generated_class())
    ga.set_editor_property("cast_montage", montage)
    ga.set_editor_property("pattern_data", da)
    ga.set_editor_property("cooldown_duration", 8.0)
    ga.set_editor_property("cooldown_tags", tags("Cooldown.Boss.Conductor.Wall"))
    for field in ("asset_tags", "ability_tags"):
        try:
            ga.set_editor_property(field, tags(ability_tag))
            break
        except Exception:
            continue
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
    for k, v in dict(ability_tag=tag(ability_tag), min_range=400.0, max_range=6000.0, preferred_range=1200.0, weight=1.0).items():
        entry.set_editor_property(k, v)
    patterns.append(entry)
    selector.set_editor_property("patterns", patterns)
    unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
    eal.save_loaded_asset(boss_bp)

    print("색 벽: 몽타주", round(montage.get_play_length(), 2), "초, 마지막 벽", da.get_last_shot_time() if hasattr(da, "get_last_shot_time") else "-",
          "/ 패턴:", [str(p.get_editor_property("ability_tag").get_editor_property("tag_name")) for p in patterns],
          "/ 능력 수:", len(abilities))
    print("CONDUCTOR_WALL_COMPLETE")


run()
