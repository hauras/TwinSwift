# 강화 카드 화면 WBP_UpgradeSelect 만들기 (부모 USUpgradeSelectWidget, 로직은 C++. 여기서는 배치만)
# Root(캔버스)
#  ├ Dim       화면 전체 반투명 검정
#  ├ Heading   "강화를 하나 고르세요" (가운데 위)
#  ├ Cards     카드 3장 가로 (가운데)
#  │   └ Card0Size(260×320) > Card0Button > Card0Content(세로) > Card0Title / Card0Target / Card0Description … Card2
#  └ Hint      "마우스로 고르거나 1 / 2 / 3" (가운데 아래)
# 이름은 C++이 GetWidgetFromName으로 찾는 규칙과 같아야 함. 글꼴은 플레이어 HUD 글꼴(한글)
# 다시 실행해도 됨 (있는 위젯은 값만 다시 넣음)
import unreal

PATH = "/Game/UI/WBP_UpgradeSelect"


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    eal = unreal.EditorAssetLibrary
    lib = unreal.TSWidgetEditLibrary

    if eal.does_asset_exist(PATH):
        wbp = unreal.load_asset(PATH)
    else:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/TwinSwift.SUpgradeSelectWidget"))
        wbp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("WBP_UpgradeSelect", "/Game/UI", unreal.WidgetBlueprint, factory)

    root = lib.ensure_root_canvas(wbp, "Root")
    base_font = lib.find_widget(unreal.load_asset("/Game/UI/WBP_TwinHUD"), "PerfectEvadeText").get_editor_property("font")

    def font(size):
        f = unreal.SlateFontInfo()
        f.import_text(base_font.export_text())
        f.set_editor_property("size", size)
        return f

    def text(parent, name, value, size, color):
        t = lib.add_child_widget(wbp, parent, unreal.TextBlock, name)
        t.set_editor_property("text", unreal.Text(value))
        t.set_editor_property("font", font(size))
        t.set_editor_property("justification", unreal.TextJustify.CENTER)
        t.set_editor_property("color_and_opacity", unreal.SlateColor(color))
        t.set_editor_property("auto_wrap_text", True)
        return t

    def place(widget, x, y):
        slot = widget.get_editor_property("slot")
        slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(x, y), maximum=unreal.Vector2D(x, y)))
        slot.set_alignment(unreal.Vector2D(0.5, 0.5))
        slot.set_auto_size(True)

    # 화면 전체 반투명 검정 (뒤 게임 화면이 비침)
    dim = lib.add_child_widget(wbp, root, unreal.Border, "Dim")
    dim.set_editor_property("brush_color", unreal.LinearColor(0.0, 0.0, 0.0, 0.55))
    dim_slot = dim.get_editor_property("slot")
    dim_slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
    dim_slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))

    place(text(root, "Heading", "강화를 하나 고르세요", 36, unreal.LinearColor(1.0, 0.95, 0.85, 1.0)), 0.5, 0.25)
    place(text(root, "Hint", "마우스로 고르거나 1 / 2 / 3", 18, unreal.LinearColor(0.75, 0.73, 0.68, 1.0)), 0.5, 0.78)

    cards = lib.add_child_widget(wbp, root, unreal.HorizontalBox, "Cards")
    place(cards, 0.5, 0.52)

    for index in range(3):
        prefix = f"Card{index}"
        size = lib.add_child_widget(wbp, "Cards", unreal.SizeBox, f"{prefix}Size")
        size.set_editor_property("width_override", 260.0)
        size.set_editor_property("height_override", 320.0)
        size.get_editor_property("slot").set_padding(unreal.Margin(12.0, 0.0, 12.0, 0.0))

        lib.add_child_widget(wbp, f"{prefix}Size", unreal.Button, f"{prefix}Button")
        content = lib.add_child_widget(wbp, f"{prefix}Button", unreal.VerticalBox, f"{prefix}Content")
        content_slot = content.get_editor_property("slot")
        content_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        content_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

        title = text(f"{prefix}Content", f"{prefix}Title", "카드 제목", 28, unreal.LinearColor(0.08, 0.08, 0.08, 1.0))
        target = text(f"{prefix}Content", f"{prefix}Target", "공통", 16, unreal.LinearColor(0.35, 0.33, 0.3, 1.0))
        description = text(f"{prefix}Content", f"{prefix}Description", "효과 설명", 18, unreal.LinearColor(0.2, 0.2, 0.2, 1.0))
        for widget, top in ((title, 0.0), (target, 8.0), (description, 18.0)):
            widget.get_editor_property("slot").set_padding(unreal.Margin(12.0, top, 12.0, 0.0))

    lib.mark_modified(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    eal.save_loaded_asset(wbp, only_if_is_dirty=False)
    print(lib.dump_widget_tree(wbp))
    print("UPGRADE_SELECT_COMPLETE")


run()
