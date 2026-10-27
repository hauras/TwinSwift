# 런 진행: 레벨에 보스를 놓지 않고 게임 모드가 차례마다 생성
# 1) BP_GameMode: 보스 순서 BP_Boss_Golem → BP_Boss_Conductor, 시작 0번, 다음 보스까지 3초
# 2) L_Dev: 놓여 있는 보스 액터 삭제 (전투 진행 액터의 보스 칸은 코드에서 없어짐), 레벨이 BP_GameMode를 쓰는지 확인
# APPLY = False면 지금 상태만 출력
import unreal

APPLY = False


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 못 고침. PIE를 끄고 다시 실행")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    gm_bp = unreal.load_asset("/Game/Blueprints/Core/BP_GameMode")
    gm = unreal.get_default_object(gm_bp.generated_class())
    golem = unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Golem").generated_class()
    conductor = unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Conductor").generated_class()

    placed = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.BossCharacter)]
    settings = world.get_world_settings()
    print("레벨:", world.get_name(), "/ 게임 모드 덮어쓰기:", settings.get_editor_property("default_game_mode"))
    print("놓인 보스:", [a.get_actor_label() for a in placed])
    print("지금 보스 순서:", [c.get_name() for c in gm.get_editor_property("boss_character_classes")])

    if not APPLY:
        print("APPLY = False: 바꾸지 않음")
        return

    gm.set_editor_property("boss_character_classes", [golem, conductor])
    gm.set_editor_property("start_index", 0)
    gm.set_editor_property("next_boss_delay", 3.0)
    unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(gm_bp)

    for actor in placed:
        actors.destroy_actor(actor)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("보스 순서:", [c.get_name() for c in gm.get_editor_property("boss_character_classes")], "/ 지운 보스", len(placed))
    print("RUN_SETUP_COMPLETE")


run()
