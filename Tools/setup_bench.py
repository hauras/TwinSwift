# 측정용: BP_PlayerController.BenchProjectileClass = 화염탄 BP (콘솔 BenchProjectiles가 쏠 투사체)
# 조건: BenchProjectiles가 들어간 C++, PIE 꺼짐. 다시 실행해도 같은 결과
import unreal

if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError("PIE를 끄고 다시 실행")

CONTROLLER_BP = "/Game/Blueprints/Core/BP_PlayerController"
FIREBOLT_BP = "/Game/Blueprints/Projectile/BP_Projectile_FireBolt"

bp = unreal.load_asset(CONTROLLER_BP)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("bench_projectile_class", unreal.load_asset(FIREBOLT_BP).generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
print("bench", cdo.get_editor_property("bench_projectile_class").get_name())
