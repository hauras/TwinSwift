# 단계 8-1 머리 위 표식 아이콘: 적 블루프린트의 MarkIndicator 컴포넌트에 아이콘 머티리얼 지정
#   [균열] = 내려찍기 아이콘(M_UI_GroundSlam, 표식을 거는 스킬), [빙결] = 냉기 스킬 아이콘(M_UI_FrostBolt)
# 조건: USMarkIndicatorComponent가 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
ENEMIES = ["/Game/Blueprints/Enemy/BP_Dummy"]

for path in ENEMIES:
    bp = unreal.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    indicator = cdo.get_editor_property("mark_indicator")
    indicator.set_editor_property("crack_icon", unreal.load_asset("/Game/UI/Icons/M_UI_GroundSlam"))
    indicator.set_editor_property("frost_icon", unreal.load_asset("/Game/UI/Icons/M_UI_FrostBolt"))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    print(path, indicator.get_editor_property("crack_icon").get_name(), indicator.get_editor_property("frost_icon").get_name())
