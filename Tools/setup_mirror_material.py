# 거울 보스 재질 두 개
# M_MirrorBody: 검은 유리(불투명, 금속성, 매끈) + 가장자리만 보라로 빛남(프레넬)
#   반투명을 쓰지 않는 이유: 겹침 순서 꼬임·그리기 비용·나나이트 제약, 무엇보다 동작이 잘 안 보임 (동작을 읽는 보스)
# M_MirrorGuard: 같은 몸 방어 중에 몸 위에 겹쳐 그림 (SetOverlayMaterial). 반투명 은백색 윤곽
# 둘 다 파라미터 없이 고정값만: 마법사 메시(Air_Wizard)가 파라미터가 하나라도 있는 재질을 원래 재질로 그림
#   (PIE에서 확인: 고정값만 있는 재질은 마법사에 잘 입혀지고, 파라미터 하나만 넣어도 원래 하늘색. 기사 메시는 둘 다 됨)
# 다시 실행해도 됨 (노드를 지우고 다시 만듦)
# 예전 M_MirrorGlass는 파라미터 노드를 지워도 일부가 남아(delete_all_material_expressions가 놓침) 새 이름으로 만듦
import unreal

FOLDER = "/Game/Assets/Characters/Boss/Mirror/Materials"

# 몸: 거의 검정, 금속성 높고 매끈 → 주변 조명이 날카롭게 비침 (유리·흑요석 느낌)
BASE_COLOR = unreal.LinearColor(0.015, 0.015, 0.025, 1.0)
METALLIC = 0.85
ROUGHNESS = 0.18

# 가장자리 빛: 보라 (주황·파랑은 탄 색 규칙에 쓰여서 겹치지 않게). 정면은 어둡고 윤곽만 빛나 실루엣이 또렷함
RIM_COLOR = unreal.LinearColor(0.55, 0.22, 1.0, 1.0)
RIM_INTENSITY = 10.0
RIM_EXPONENT = 2.0

# 방어 겹침: 몸 전체를 은백색 막으로 덮고 가장자리는 더 밝게 ("지금은 안 깨짐"). 평소의 검정+보라와 한눈에 갈리게
GUARD_COLOR = unreal.LinearColor(0.85, 0.9, 1.0, 1.0)
GUARD_BASE_GLOW = 2.0
GUARD_EDGE_GLOW = 10.0
GUARD_BASE_OPACITY = 0.55
GUARD_EDGE_OPACITY = 0.45
GUARD_EXPONENT = 1.5

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def load_or_create(name):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        mat = unreal.load_asset(path)
        mel.delete_all_material_expressions(mat)
        return mat
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def build(mat, fill):
    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def scalar(value, x, y):
        n = node(unreal.MaterialExpressionConstant, x, y)
        n.set_editor_property("r", value)
        return n

    def color(value, x, y):
        n = node(unreal.MaterialExpressionConstant3Vector, x, y)
        n.set_editor_property("constant", value)
        return n

    def multiply(a, b, x, y):
        n = node(unreal.MaterialExpressionMultiply, x, y)
        mel.connect_material_expressions(a, "", n, "A")
        mel.connect_material_expressions(b, "", n, "B")
        return n

    def fresnel(exponent, x, y):
        n = node(unreal.MaterialExpressionFresnel, x, y)
        n.set_editor_property("exponent", exponent)
        n.set_editor_property("base_reflect_fraction", 0.0)
        return n

    fill(node, scalar, color, multiply, fresnel)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)
    print("재질:", mat.get_path_name())


def glass(node, scalar, color, multiply, fresnel):
    mel.connect_material_property(color(BASE_COLOR, -800, -200), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(METALLIC, -800, 0), "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(scalar(ROUGHNESS, -800, 100), "", unreal.MaterialProperty.MP_ROUGHNESS)
    rim = multiply(multiply(color(RIM_COLOR, -800, 250), fresnel(RIM_EXPONENT, -800, 400), -500, 300), scalar(RIM_INTENSITY, -800, 550), -300, 350)
    mel.connect_material_property(rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def guard(node, scalar, color, multiply, fresnel):
    def add(a, b, x, y):
        n = node(unreal.MaterialExpressionAdd, x, y)
        mel.connect_material_expressions(a, "", n, "A")
        mel.connect_material_expressions(b, "", n, "B")
        return n

    # 밝기 = 바탕 + 가장자리 × 세기 / 불투명도 = 바탕 + 가장자리 × 더할 값 (정면도 막이 보이게)
    edge = fresnel(GUARD_EXPONENT, -900, 300)
    strength = add(scalar(GUARD_BASE_GLOW, -900, 450), multiply(edge, scalar(GUARD_EDGE_GLOW, -900, 550), -650, 500), -450, 450)
    mel.connect_material_property(multiply(color(GUARD_COLOR, -900, 100), strength, -250, 250), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = add(scalar(GUARD_BASE_OPACITY, -900, 700), multiply(edge, scalar(GUARD_EDGE_OPACITY, -900, 800), -650, 750), -450, 700)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    mat = load_or_create("M_MirrorBody")
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    build(mat, glass)

    mat = load_or_create("M_MirrorGuard")
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    build(mat, guard)

    print("MIRROR_MATERIAL_COMPLETE")


run()
