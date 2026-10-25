# 단계 6 시험용: 허수아비(BP_Dummy)의 주기 공격 켜기/끄기
# 켜면 3초마다 바닥에 빨간 원 경고 → 안쪽 원이 바깥 원에 닿는 순간 원 안의 플레이어에게 데미지 20
# 보스 AI가 생기면 ENABLE = False로 다시 실행
import unreal

ENABLE = True

bp = unreal.load_asset("/Game/Blueprints/Debug/BP_Dummy.BP_Dummy")
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("debug_auto_attack", ENABLE)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)

print("BP_Dummy 주기 공격:", cdo.get_editor_property("debug_auto_attack"),
      "| 간격", cdo.get_editor_property("debug_attack_interval"),
      "| 경고", cdo.get_editor_property("debug_attack_windup"),
      "| 반경", cdo.get_editor_property("debug_attack_radius"),
      "| 배율", cdo.get_editor_property("debug_attack_multiplier"))
