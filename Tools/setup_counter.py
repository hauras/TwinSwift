# 반격 기회: 충격파를 완벽 회피(교체·회피 직후 0.25초)로 받으면 지휘자가 2.5초 큰 경직 (피해 1.5배)
# DA_Conductor_Shockwave: 반격 가능 켬. 경직 시간은 보스 기본값(CounterGroggyDuration 2.5)
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    da = unreal.load_asset("/Game/Abilities/Boss/Patterns/DA_Conductor_Shockwave")
    events = list(da.get_editor_property("events"))
    for event in events:
        event.set_editor_property("counterable", True)
    da.set_editor_property("events", events)
    unreal.EditorAssetLibrary.save_loaded_asset(da, only_if_is_dirty=False)

    boss = unreal.get_default_object(unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Conductor").generated_class())
    print("반격 가능:", [e.get_editor_property("counterable") for e in events],
          "/ 경직 시간:", boss.get_editor_property("counter_groggy_duration"))
    print("COUNTER_COMPLETE")


run()
