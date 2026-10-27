# 지휘자 크기 2배: 키 2m → 4m (플레이어 약 1.8m, 골렘 2.8m)
# 캡슐과 메시를 같은 비율로, 떠 있는 높이(메시 Z)도 같은 비율. 몸이 커진 만큼 충격파 "붙어 있음" 거리 350 → 450
# 다시 실행해도 같은 값 (배율을 곱하지 않고 값을 정함)
import unreal

SCALE = 2.0


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    boss_bp = unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Conductor")
    boss = unreal.get_default_object(boss_bp.generated_class())

    capsule = boss.get_editor_property("capsule_component")
    capsule.set_editor_property("capsule_radius", 45.0 * SCALE)
    capsule.set_editor_property("capsule_half_height", 100.0 * SCALE)

    mesh = boss.get_editor_property("mesh")
    mesh.set_editor_property("relative_scale3d", unreal.Vector(SCALE, SCALE, SCALE))
    mesh.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -50.0 * SCALE))

    selector = boss.get_editor_property("pattern_component")
    patterns = list(selector.get_editor_property("patterns"))
    for pattern in patterns:
        if str(pattern.get_editor_property("ability_tag").get_editor_property("tag_name")) == "Ability.Boss.Conductor.Shockwave":
            pattern.set_editor_property("max_range", 450.0)
    selector.set_editor_property("patterns", patterns)

    unreal.BlueprintEditorLibrary.compile_blueprint(boss_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(boss_bp)

    print("캡슐", capsule.get_editor_property("capsule_radius"), capsule.get_editor_property("capsule_half_height"),
          "/ 메시", mesh.get_editor_property("relative_scale3d"), mesh.get_editor_property("relative_location").z)
    print("CONDUCTOR_SCALE_COMPLETE")


run()
