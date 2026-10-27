# 강화 카드 화면 꾸미기 (WBP_UpgradeSelect, 배치는 setup_upgrade_select.py)
# 카드: 어두운 판 + 둥근 모서리 + 얇은 테두리 → 마우스를 올리면 테두리가 금색으로 굵어지고 판이 밝아짐, 누르면 살짝 어두워짐
# 글자: 제목 밝은 흰색 크게, 대상은 코드가 색을 정함(전사 주황·마법사 파랑·공통 회색), 설명 연한 회색
# 다시 실행해도 됨
import unreal

PATH = "/Game/UI/WBP_UpgradeSelect"

CARD = unreal.LinearColor(0.06, 0.06, 0.08, 0.96)
CARD_HOVER = unreal.LinearColor(0.11, 0.10, 0.12, 0.98)
CARD_PRESSED = unreal.LinearColor(0.04, 0.04, 0.05, 0.98)
EDGE = unreal.LinearColor(0.35, 0.33, 0.30, 1.0)
EDGE_HOVER = unreal.LinearColor(1.0, 0.78, 0.38, 1.0)
RADIUS = 18.0


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    lib = unreal.TSWidgetEditLibrary
    wbp = unreal.load_asset(PATH)

    def brush(fill, edge, width):
        b = unreal.SlateBrush()
        b.set_editor_property("draw_as", unreal.SlateBrushDrawType.ROUNDED_BOX)
        b.set_editor_property("tint_color", unreal.SlateColor(fill))
        outline = unreal.SlateBrushOutlineSettings()
        outline.set_editor_property("corner_radii", unreal.Vector4(RADIUS, RADIUS, RADIUS, RADIUS))
        outline.set_editor_property("color", unreal.SlateColor(edge))
        outline.set_editor_property("width", width)
        outline.set_editor_property("rounding_type", unreal.SlateBrushRoundingType.FIXED_RADIUS)
        b.set_editor_property("outline_settings", outline)
        return b

    for index in range(3):
        prefix = f"Card{index}"
        button = lib.find_widget(wbp, f"{prefix}Button")
        style = button.get_editor_property("widget_style")
        style.set_editor_property("normal", brush(CARD, EDGE, 1.5))
        style.set_editor_property("hovered", brush(CARD_HOVER, EDGE_HOVER, 3.0))
        style.set_editor_property("pressed", brush(CARD_PRESSED, EDGE_HOVER, 3.0))
        style.set_editor_property("normal_padding", unreal.Margin(16.0, 20.0, 16.0, 20.0))
        style.set_editor_property("pressed_padding", unreal.Margin(16.0, 22.0, 16.0, 18.0))
        button.set_editor_property("widget_style", style)

        title = lib.find_widget(wbp, f"{prefix}Title")
        title.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 0.96, 0.9, 1.0)))
        description = lib.find_widget(wbp, f"{prefix}Description")
        description.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(0.78, 0.76, 0.72, 1.0)))
        # 대상 글자는 코드가 대상에 따라 색을 넣음 (기본은 회색)
        target = lib.find_widget(wbp, f"{prefix}Target")
        target.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(0.7, 0.68, 0.64, 1.0)))

        # 제목·대상·설명 사이 간격: 대상 아래를 넓혀 설명과 구분
        description.get_editor_property("slot").set_padding(unreal.Margin(8.0, 24.0, 8.0, 0.0))

    # 위아래 안내 글자는 카드보다 차분하게
    lib.find_widget(wbp, "Heading").set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 0.86, 0.6, 1.0)))
    lib.find_widget(wbp, "Dim").set_editor_property("brush_color", unreal.LinearColor(0.0, 0.0, 0.02, 0.65))

    lib.mark_modified(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp, only_if_is_dirty=False)
    print("UPGRADE_SELECT_STYLED")


run()
