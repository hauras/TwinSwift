# 단계 8-3 균열 폭발 이펙트: NS_Dark_Stone_Impact(돌 파편)를 복사해 주황색 NS_CrackBurst로 → 낙뢰 BurstEffect
#   [균열] 아이콘·주황 피해 숫자와 색을 맞춤. 원본(외부 팩)은 건드리지 않음
#   색은 입자 색(모듈 색·색 커브)만 돌림. 머티리얼은 다른 이펙트와 같이 써서 그대로
# 조건: UTSWeaponTrailLibrary.TintSystemHue가 들어간 C++. 다시 실행해도 같은 결과 (있으면 복사하지 않고 색만 다시)
import unreal

eal = unreal.EditorAssetLibrary
SOURCE = "/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Stone_Impact"
TARGET = "/Game/Assets/VFX/Marks/NS_CrackBurst"
THUNDER = "/Game/Abilities/Mage/GA_Mage_Thunder"

# 주황 (색상환 25도)
TARGET_HUE = 25.0

if not eal.does_asset_exist(TARGET):
    if not eal.duplicate_asset(SOURCE, TARGET):
        raise RuntimeError("복사 실패: " + SOURCE)

system = unreal.load_asset(TARGET)
report = unreal.TSWeaponTrailLibrary.tint_system_hue(system, TARGET_HUE, True)
print(report)
eal.save_loaded_asset(system, only_if_is_dirty=False)

bp = unreal.load_asset(THUNDER)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("burst_effect", system)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)
print(THUNDER, unreal.get_default_object(bp.generated_class()).get_editor_property("burst_effect").get_path_name())
