# 단계 8-2 산산조각: 전사 능력(평타·돌진 베기·내려찍기)이 [빙결]을 깨게 함
#   bShattersFrost 켬 + 파편 이펙트 NS_FrostShatter. 배율 3.0·경직 0.5초는 C++ 기본값
# 조건: USGameplayAbility에 산산조각 속성이 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
ABILITIES = [
    "/Game/Abilities/Warrior/GA_Warrior_Attack",
    "/Game/Abilities/Warrior/GA_Warrior_DashSlash",
    "/Game/Abilities/Warrior/GA_Warrior_GroundSlam",
]
SHATTER_FX = "/Game/Assets/VFX/Marks/NS_FrostShatter"

effect = unreal.load_asset(SHATTER_FX)
if not effect:
    raise RuntimeError("이펙트 없음: " + SHATTER_FX)

for path in ABILITIES:
    bp = unreal.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("shatters_frost", True)
    cdo.set_editor_property("shatter_effect", effect)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)

    cdo = unreal.get_default_object(bp.generated_class())
    print(path,
          cdo.get_editor_property("shatters_frost"),
          cdo.get_editor_property("shatter_multiplier"),
          cdo.get_editor_property("shatter_stun_duration"),
          cdo.get_editor_property("shatter_effect").get_name())
