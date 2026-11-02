# 거울 보스 2페이즈 설정 (C++ 빌드 후): 반전 예고 겹침 재질, 2페이즈 안내 글자, 화면 흔들림
# 들어가는 체력 비율은 보스 기본값(PhaseTwoHealthRatio 0.4) 그대로
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

bp = unreal.load_asset("/Game/Blueprints/Boss/BP_Boss_Mirror")
boss = unreal.get_default_object(bp.generated_class())
boss.set_editor_property("reverse_warn_overlay_material", unreal.load_asset("/Game/Assets/Characters/Boss/Mirror/Materials/M_MirrorReverse"))
boss.set_editor_property("phase_two_message", unreal.Text("거울이 뒤집힌다"))
boss.set_editor_property("phase_two_shake", unreal.load_class(None, "/Script/TwinSwift.SCameraShake_BossImpact"))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)

boss = unreal.get_default_object(bp.generated_class())
print("반전 예고:", boss.get_editor_property("reverse_warn_overlay_material").get_name())
print("안내:", boss.get_editor_property("phase_two_message"), "/ 흔들림:", boss.get_editor_property("phase_two_shake").get_name())
print("2페이즈 체력:", boss.get_editor_property("phase_two_health_ratio"))
print("MIRROR_PHASE2_COMPLETE")
