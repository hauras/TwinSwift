# 지휘자 2페이즈: 체력 50%에서 패턴 선택기 페이즈 2 (진입 능력 없음 → 패턴만 바뀜)
# 한 방향 색 벽은 1페이즈 전용 (2페이즈에서는 협공 벽으로 바뀜)
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    boss_bp = unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Conductor")
    boss = unreal.get_default_object(boss_bp.generated_class())
    boss.set_editor_property("phase_two_health_ratio", 0.5)

    selector = boss.get_editor_property("pattern_component")
    patterns = list(selector.get_editor_property("patterns"))
    for pattern in patterns:
        if str(pattern.get_editor_property("ability_tag").get_editor_property("tag_name")) == "Ability.Boss.Conductor.Wall":
            pattern.set_editor_property("max_phase", 1)
    selector.set_editor_property("patterns", patterns)

    unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(boss_bp)
    print("CONDUCTOR_PHASE2_COMPLETE")


run()
