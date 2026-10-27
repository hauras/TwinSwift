# 강화 카드 장식 (WBP_UpgradeSelect, 판·테두리는 style_upgrade_select.py)
# 카드 안 순서: 키 표시("1") → 대상 색띠(코드가 색을 넣음) → 제목 → 대상 → 구분선 → 설명
#   Card0Key / Card0BandSize > Card0Band / Card0Title / Card0Target / Card0DividerSize > Card0Divider / Card0Description
# 새로 넣은 위젯은 끝에 붙으므로, 빼고 위 순서로 다시 넣어 정렬
# 다시 실행해도 됨
import unreal

PATH = "/Game/UI/WBP_UpgradeSelect"


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    lib = unreal.TSWidgetEditLibrary
    wbp = unreal.load_asset(PATH)
    base_font = lib.find_widget(unreal.load_asset("/Game/UI/WBP_TwinHUD"), "PerfectEvadeText").get_editor_property("font")

    def font(size):
        f = unreal.SlateFontInfo()
        f.import_text(base_font.export_text())
        f.set_editor_property("size", size)
        return f

    def bar(content, size_name, bar_name, height, color):
        size = lib.add_child_widget(wbp, content, unreal.SizeBox, size_name)
        size.set_editor_property("height_override", height)
        border = lib.add_child_widget(wbp, size_name, unreal.Border, bar_name)
        border.set_editor_property("brush_color", color)
        return size

    for index in range(3):
        prefix = f"Card{index}"
        content_name = f"{prefix}Content"
        content = lib.find_widget(wbp, content_name)

        # 키 표시: 왼쪽 위 작은 금색 숫자 (키보드 1/2/3)
        key = lib.add_child_widget(wbp, content_name, unreal.TextBlock, f"{prefix}Key")
        key.set_editor_property("text", unreal.Text(str(index + 1)))
        key.set_editor_property("font", font(16))
        key.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 0.78, 0.38, 0.9)))

        # 대상 색띠 (전사 주황·마법사 파랑·공통 회색, 코드가 칠함) / 제목·대상과 설명 사이 구분선
        band = bar(content_name, f"{prefix}BandSize", f"{prefix}Band", 6.0, unreal.LinearColor(0.7, 0.68, 0.64, 1.0))
        divider = bar(content_name, f"{prefix}DividerSize", f"{prefix}Divider", 1.0, unreal.LinearColor(1.0, 1.0, 1.0, 0.15))

        title = lib.find_widget(wbp, f"{prefix}Title")
        target = lib.find_widget(wbp, f"{prefix}Target")
        description = lib.find_widget(wbp, f"{prefix}Description")
        title.set_editor_property("font", font(30))

        # 순서 맞추기: 전부 뺐다가 정해진 순서로 다시 넣음 (위젯 자체와 값은 그대로)
        order = [key, band, title, target, divider, description]
        for widget in order:
            content.remove_child(widget)
        for widget in order:
            content.add_child(widget)

        # 간격 (왼쪽, 위, 오른쪽, 아래)
        paddings = {
            key: unreal.Margin(4.0, 0.0, 4.0, 0.0),
            band: unreal.Margin(40.0, 10.0, 40.0, 0.0),
            title: unreal.Margin(8.0, 20.0, 8.0, 0.0),
            target: unreal.Margin(8.0, 6.0, 8.0, 0.0),
            divider: unreal.Margin(24.0, 18.0, 24.0, 0.0),
            description: unreal.Margin(8.0, 18.0, 8.0, 0.0),
        }
        for widget, margin in paddings.items():
            slot = widget.get_editor_property("slot")
            slot.set_padding(margin)
            slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_LEFT if widget == key else unreal.HorizontalAlignment.H_ALIGN_FILL)

    lib.mark_modified(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp, only_if_is_dirty=False)
    print("\n".join(l for l in lib.dump_widget_tree(wbp).splitlines() if "Card0" in l))
    print("UPGRADE_CARDS_DECORATED")


run()
