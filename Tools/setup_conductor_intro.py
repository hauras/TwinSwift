# 지휘자 등장신 "연주 시작" + 2페이즈 알림
# 1) DA_Conductor_Intro: 첫 박(1.31초, 손을 펴는 순간)에 고리 2줄 (주황 16 + 파랑 16, 반 칸 엇갈림), 느리게(500), 짧게(1.2초 ≈ 7m), 피해 0
# 2) GA_Conductor_Intro (부모 SGA_ConductorIntro): 몽타주는 충격파와 같은 AM_Conductor_Shockwave (손을 모았다 펴는 동작 → 충격파 예고를 미리 보여 줌)
# 3) BP_Boss_Conductor: 시작 능력 + IntroAbilityTag, 2페이즈 안내 글자·화면 흔들림
# 4) 레벨(L_Dev): Encounter_Conductor 등장신 켬 + 등장 카메라 (플레이어 시작 쪽 높은 곳에서 경기장 가운데를 내려다봄)
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import math
import unreal

CAMERA_BACK = 1500.0
CAMERA_HEIGHT = 900.0


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    da_path = "/Game/Abilities/Boss/Patterns/DA_Conductor_Intro"
    ga_path = "/Game/Abilities/Boss/GA_Conductor_Intro"
    boss_path = "/Game/Blueprints/Boss/BP_Boss_Conductor"
    ability_tag = "Ability.Boss.Conductor.Intro"

    def tags(*names):
        c = unreal.GameplayTagContainer()
        c.import_text("(GameplayTags=(" + ",".join(f'(TagName="{n}")' for n in names) + "))")
        return c

    def tag(name):
        t = unreal.GameplayTag()
        t.import_text(f'(TagName="{name}")')
        return t

    # 1) 첫 박 데이터
    if eal.does_asset_exist(da_path):
        da = unreal.load_asset(da_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.SBulletPatternData)
        da = tools.create_asset("DA_Conductor_Intro", "/Game/Abilities/Boss/Patterns", unreal.SBulletPatternData, factory)

    def ring(color, angle):
        row = unreal.SBulletPatternEvent()
        for k, v in dict(fire_time=1.31, bullet_count=16, shape=unreal.BulletShape.RING, speed=500.0, angle_offset=angle,
                         color=color, repeat_count=1, damage_multiplier=0.0, life_time=1.2).items():
            row.set_editor_property(k, v)
        return row

    da.set_editor_property("events", [ring(unreal.BulletColor.ORANGE, 0.0), ring(unreal.BulletColor.BLUE, 11.25)])
    eal.save_loaded_asset(da, only_if_is_dirty=False)

    # 2) 등장 능력
    shockwave_ga = unreal.get_default_object(unreal.load_asset("/Game/Abilities/Boss/GA_Conductor_Shockwave").generated_class())
    if eal.does_asset_exist(ga_path):
        ga_bp = unreal.load_asset(ga_path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SGA_ConductorIntro"))
        ga_bp = tools.create_asset("GA_Conductor_Intro", "/Game/Abilities/Boss", unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    ga = unreal.get_default_object(ga_bp.generated_class())
    ga.set_editor_property("cast_montage", unreal.load_asset("/Game/Assets/Characters/Boss/Conductor/Montages/AM_Conductor_Shockwave"))
    ga.set_editor_property("pattern_data", da)
    ga.set_editor_property("orange_telegraph_material", shockwave_ga.get_editor_property("orange_telegraph_material"))
    ga.set_editor_property("blue_telegraph_material", shockwave_ga.get_editor_property("blue_telegraph_material"))
    ga.set_editor_property("ability_tags", tags(ability_tag))
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    eal.save_loaded_asset(ga_bp)

    # 3) 보스
    boss_bp = unreal.load_asset(boss_path)
    boss = unreal.get_default_object(boss_bp.generated_class())
    abilities = list(boss.get_editor_property("startup_abilities"))
    if ga_bp.generated_class() not in abilities:
        abilities.append(ga_bp.generated_class())
    boss.set_editor_property("startup_abilities", abilities)
    boss.set_editor_property("intro_ability_tag", tag(ability_tag))
    boss.set_editor_property("phase_two_message", unreal.Text("지휘자가 템포를 올린다"))
    boss.set_editor_property("phase_two_shake", unreal.load_class(None, "/Script/TwinSwift.SCameraShake_BossImpact"))
    unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
    eal.save_loaded_asset(boss_bp)

    # 4) 레벨: 등장신 켬 + 카메라
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    all_actors = actors.get_all_level_actors()
    encounter = next(a for a in all_actors if isinstance(a, unreal.SBossEncounter) and a.get_editor_property("boss")
                     and "Conductor" in a.get_editor_property("boss").get_name())
    player_start = next(a for a in all_actors if isinstance(a, unreal.PlayerStart))
    center = encounter.get_actor_location()
    ps = player_start.get_actor_location()
    to_player = unreal.Vector(ps.x - center.x, ps.y - center.y, 0.0)
    if to_player.length() < 1.0:
        to_player = unreal.Vector(-1.0, 0.0, 0.0)
    to_player = to_player.normal()

    boss_z = encounter.get_editor_property("boss").get_actor_location().z
    cam = unreal.Vector(center.x + to_player.x * CAMERA_BACK, center.y + to_player.y * CAMERA_BACK, boss_z + CAMERA_HEIGHT)
    look = unreal.Vector(center.x - cam.x, center.y - cam.y, boss_z - cam.z)
    rotation = unreal.Rotator(roll=0.0,
                              pitch=math.degrees(math.atan2(look.z, math.hypot(look.x, look.y))),
                              yaw=math.degrees(math.atan2(look.y, look.x)))

    camera = next((a for a in all_actors if isinstance(a, unreal.CameraActor) and a.get_actor_label() == "IntroCamera_Conductor"), None)
    if not camera:
        camera = actors.spawn_actor_from_class(unreal.CameraActor, cam)
        camera.set_actor_label("IntroCamera_Conductor")
    camera.set_actor_location(cam, False, False)
    camera.set_actor_rotation(rotation, False)
    encounter.set_editor_property("intro_camera", camera)
    encounter.set_editor_property("play_intro", True)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()

    print("등장: 능력", len(abilities), "개, 카메라", cam, "/ 등장신", encounter.get_editor_property("play_intro"))
    print("CONDUCTOR_INTRO_COMPLETE")


run()
