# 거울 보스 재질 M_MirrorGlass: 검은 유리(불투명, 금속성, 매끈) + 가장자리만 보라로 빛남(프레넬)
# 반투명을 쓰지 않는 이유: 겹침 순서 꼬임·그리기 비용·나나이트 제약, 무엇보다 동작이 잘 안 보임 (동작을 읽는 보스)
# 파라미터: BaseColor, RimColor, RimIntensity, RimExponent, Roughness → 에디터에서 인스턴스로 조절
# 다시 실행해도 됨 (노드를 지우고 다시 만듦)
import unreal

PATH = "/Game/Assets/Characters/Boss/Mirror/Materials/M_MirrorGlass"


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    mel = unreal.MaterialEditingLibrary
    folder, name = PATH.rsplit("/", 1)
    if eal.does_asset_exist(PATH):
        mat = unreal.load_asset(PATH)
        mel.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    # 스켈레탈 메시에 씀
    mat.set_editor_property("used_with_skeletal_mesh", True)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def scalar(param, value, x, y):
        n = node(unreal.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", param)
        n.set_editor_property("default_value", value)
        return n

    def vector(param, color, x, y):
        n = node(unreal.MaterialExpressionVectorParameter, x, y)
        n.set_editor_property("parameter_name", param)
        n.set_editor_property("default_value", color)
        return n

    def multiply(a, b, x, y):
        n = node(unreal.MaterialExpressionMultiply, x, y)
        mel.connect_material_expressions(a, "", n, "A")
        mel.connect_material_expressions(b, "", n, "B")
        return n

    # 몸: 거의 검정, 금속성 높고 매끈 → 주변 조명이 날카롭게 비침 (유리·흑요석 느낌)
    base = vector("BaseColor", unreal.LinearColor(0.015, 0.015, 0.025, 1.0), -800, -200)
    metallic = scalar("Metallic", 0.85, -800, 0)
    roughness = scalar("Roughness", 0.18, -800, 100)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # 가장자리 빛: 보라 (주황·파랑은 탄 색 규칙에 쓰여서 겹치지 않게). 정면은 어둡고 윤곽만 빛나 실루엣이 또렷함
    rim_color = vector("RimColor", unreal.LinearColor(0.55, 0.22, 1.0, 1.0), -800, 250)
    rim_intensity = scalar("RimIntensity", 5.0, -800, 450)
    fresnel = node(unreal.MaterialExpressionFresnel, -800, 600)
    fresnel.set_editor_property("exponent", 3.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.0)
    rim = multiply(multiply(rim_color, fresnel, -500, 350), rim_intensity, -300, 400)
    mel.connect_material_property(rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)
    print("재질:", mat.get_path_name())
    print("MIRROR_MATERIAL_COMPLETE")


run()
