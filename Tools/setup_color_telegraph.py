# 충격파 색 무작위 + 예고: 손을 모으는 동안 보스 몸 가장자리가 이번 색으로 빛남 → 보고 교체
# 1) MI_Telegraph_Orange / MI_Telegraph_Blue: 골렘 갑옷 오버레이 재질(M_GolemArmor, 가장자리 빛)의 색만 바꾼 인스턴스. 색은 탄 색과 같게
# 2) GA_Conductor_Shockwave: 예고 재질 두 칸 연결
# 3) DA_Conductor_Shockwave: 무작위 색 켬
# 다시 실행해도 됨. 함수 안에서만 에셋을 잡음
import unreal

# setup_bullet_manager.py의 탄 색과 같은 값
ORANGE = unreal.LinearColor(1.0, 0.45, 0.08, 1.0)
BLUE = unreal.LinearColor(0.1, 0.45, 1.0, 1.0)


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    mel = unreal.MaterialEditingLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    folder = "/Game/Assets/VFX/Boss/Conductor"
    parent = unreal.load_asset("/Game/Assets/Characters/Boss/Golem/Materials/M_GolemArmor")

    # 1) 색 인스턴스
    def make_instance(name, color):
        path = f"{folder}/{name}"
        if eal.does_asset_exist(path):
            mi = unreal.load_asset(path)
        else:
            mi = tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        mel.set_material_instance_vector_parameter_value(mi, "Color", color)
        mel.set_material_instance_scalar_parameter_value(mi, "Glow", 1.0)
        eal.save_loaded_asset(mi, only_if_is_dirty=False)
        return mi

    orange = make_instance("MI_Telegraph_Orange", ORANGE)
    blue = make_instance("MI_Telegraph_Blue", BLUE)

    # 2) 능력에 연결
    ga_bp = unreal.load_asset("/Game/Abilities/Boss/GA_Conductor_Shockwave")
    ga = unreal.get_default_object(ga_bp.generated_class())
    ga.set_editor_property("orange_telegraph_material", orange)
    ga.set_editor_property("blue_telegraph_material", blue)
    unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
    eal.save_loaded_asset(ga_bp)

    # 3) 무작위 색
    da = unreal.load_asset("/Game/Abilities/Boss/Patterns/DA_Conductor_Shockwave")
    events = list(da.get_editor_property("events"))
    for event in events:
        event.set_editor_property("random_color", True)
    da.set_editor_property("events", events)
    eal.save_loaded_asset(da, only_if_is_dirty=False)

    print("예고 재질:", orange.get_name(), blue.get_name(), "/ 무작위 색:", [e.get_editor_property("random_color") for e in events])
    print("COLOR_TELEGRAPH_COMPLETE")


run()
