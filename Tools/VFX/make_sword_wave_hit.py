# 전사 어시스트 검기 명중 이펙트: 마탄 명중(NS_MageBolt_Impact: 번쩍 + 링 + 불꽃)을 복사해 금색으로 → BP_SwordWave
#   검기 반달(NE_attack05)과 같은 금색. 관통이라 지나가는 적마다 한 번씩 터짐 (ASProjectile::Pierce)
#   흰 번쩍은 채도가 없어 그대로. 원본은 건드리지 않음
# 조건: UTSWeaponTrailLibrary.TintSystemHue가 들어간 C++, PIE 꺼짐. 다시 실행해도 같은 결과
import unreal

if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError("PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
SOURCE = "/Game/Assets/VFX/Mage/NS_MageBolt_Impact"
TARGET = "/Game/Assets/VFX/Common/Assist/NS_SwordWave_Hit"
SWORD_WAVE_BP = "/Game/Blueprints/Projectile/BP_SwordWave"

# 금색 (색상환 42도)
TARGET_HUE = 42.0

if not eal.does_asset_exist(TARGET):
    if not eal.duplicate_asset(SOURCE, TARGET):
        raise RuntimeError("복사 실패: " + SOURCE)
system = unreal.load_asset(TARGET)
print(unreal.TSWeaponTrailLibrary.tint_system_hue(system, TARGET_HUE, True))
eal.save_loaded_asset(system, only_if_is_dirty=False)

wave_bp = unreal.load_asset(SWORD_WAVE_BP)
wave = unreal.get_default_object(wave_bp.generated_class())
wave.set_editor_property("impact_effect", system)
unreal.BlueprintEditorLibrary.compile_blueprint(wave_bp)
eal.save_loaded_asset(wave_bp, only_if_is_dirty=False)
print("wave impact", wave.get_editor_property("impact_effect").get_path_name())
