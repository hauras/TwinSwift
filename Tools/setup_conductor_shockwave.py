# 지휘자 근접 대응 "충격파": 붙어 있으면 짧은 예고 뒤 주변으로 빠른 탄 고리, 금방 사라짐 (코드 없이 패턴 데이터로)
# 1) AM_Conductor_Shockwave (두 손 범위 공격 애니)
# 2) DA_Conductor_Shockwave: 고리 48발(틈 없음), 속도 1500, 수명 0.35초(약 5m까지만), 색 무작위(예고 재질은 setup_color_telegraph.py), 피해 2배, 밀기 600cm
# 3) GA_Conductor_Shockwave (부모 SGA_ConductorPattern), 쿨다운 4초
# 4) BP_Boss_Conductor: 시작 능력 + 패턴 선택기에 추가 (거리 0~350, 가까우면 자주 나오게 가중치 높게)
# 다시 실행해도 됨. 끝에서 에셋을 담은 변수를 비움 (에디터 파이썬이 에셋을 붙잡지 않게)
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    base = "/Game/Assets/Characters/Boss/Conductor"
    anim_path = f"{base}/Animations/A_Conductor_2HMagicAreaAttack_02"
    montage_path = f"{base}/Montages/AM_Conductor_Shockwave"
    da_path = "/Game/Abilities/Boss/Patterns/DA_Conductor_Shockwave"
    ga_path = "/Game/Abilities/Boss/GA_Conductor_Shockwave"
    boss_path = "/Game/Blueprints/Boss/BP_Boss_Conductor"

    def tags(*names):
        c = unreal.GameplayTagContainer()
        c.import_text("(GameplayTags=(" + ",".join(f'(TagName="{n}")' for n in names) + "))")
        return c

    def tag(name):
        t = unreal.GameplayTag()
        t.import_text(f'(TagName="{name}")')
        return t

    # 몽타주
    if eal.does_asset_exist(montage_path):
        montage = unreal.load_asset(montage_path)
    else:
        factory = unreal.AnimMontageFactory()
        factory.set_editor_property("source_animation", unreal.load_asset(anim_path))
        montage = tools.create_asset("AM_Conductor_Shockwave", f"{base}/Montages", unreal.AnimMontage, factory)
    eal.save_loaded_asset(montage)

    # 첫 발 시각: 몽타주 길이의 40% (손을 모았다 펴는 순간 근처, 애니를 보고 조정). 블렌드 아웃 전이어야 함
    length = montage.get_play_length()
    fire_time = round(length * 0.4, 2)

    # 패턴 데이터
    if eal.does_asset_exist(da_path):
        da = unreal.load_asset(da_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.SBulletPatternData)
        da = tools.create_asset("DA_Conductor_Shockwave", "/Game/Abilities/Boss/Patterns", unreal.SBulletPatternData, factory)
    row = unreal.SBulletPatternEvent()
    for k, v in dict(fire_time=fire_time, bullet_count=48, shape=unreal.BulletShape.RING, speed=1500.0,
                     color=unreal.BulletColor.ORANGE, repeat_count=1, damage_multiplier=2.0, life_time=0.35,
                     knockback_strength=600.0, random_color=True).items():
        row.set_editor_property(k, v)
    da.set_editor_property("events", [row])
    eal.save_loaded_asset(da, only_if_is_dirty=False)

    # 능력 BP
    if eal.does_asset_exist(ga_path):
        ga_bp = unreal.load_asset(ga_path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SGA_ConductorPattern"))
        ga_bp = tools.create_asset("GA_Conductor_Shockwave", "/Game/Abilities/Boss", unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    ga = unreal.get_default_object(ga_bp.generated_class())
    ga.set_editor_property("cast_montage", montage)
    ga.set_editor_property("pattern_data", da)
    ga.set_editor_property("cooldown_duration", 4.0)
    ga.set_editor_property("cooldown_tags", tags("Cooldown.Boss.Conductor.Shockwave"))
    for field in ("asset_tags", "ability_tags"):
        try:
            ga.set_editor_property(field, tags("Ability.Boss.Conductor.Shockwave"))
            break
        except Exception:
            continue
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    eal.save_loaded_asset(ga_bp)

    # 보스: 시작 능력 + 패턴 목록 (같은 태그가 있으면 바꿔 넣음)
    boss_bp = unreal.load_asset(boss_path)
    boss = unreal.get_default_object(boss_bp.generated_class())
    abilities = list(boss.get_editor_property("startup_abilities"))
    if ga_bp.generated_class() not in abilities:
        abilities.append(ga_bp.generated_class())
    boss.set_editor_property("startup_abilities", abilities)

    selector = boss.get_editor_property("pattern_component")
    shock_tag = tag("Ability.Boss.Conductor.Shockwave")
    patterns = [p for p in selector.get_editor_property("patterns")
                if str(p.get_editor_property("ability_tag").get_editor_property("tag_name")) != "Ability.Boss.Conductor.Shockwave"]
    shock = unreal.SBossPattern()
    for k, v in dict(ability_tag=shock_tag, min_range=0.0, max_range=350.0, preferred_range=150.0, weight=2.0).items():
        shock.set_editor_property(k, v)
    patterns.append(shock)
    selector.set_editor_property("patterns", patterns)
    unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
    eal.save_loaded_asset(boss_bp)

    print("충격파: 몽타주", round(length, 2), "초, 첫 발", fire_time, "초, 패턴 수", len(patterns))
    print("CONDUCTOR_SHOCKWAVE_COMPLETE")


run()
