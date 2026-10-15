# WBP_TwinHUD에 교체 규칙 표시 위젯 추가 (단계 5-3)
# 각 카드(Canvas Panel 174×156) 안: 초상 영역(3, 3, 168×112)을 덮는 쿨다운 덮개 + 남은 시간 + "쓰러짐"
# 화면 가운데: "패배"
# 모두 처음엔 숨김. 보이고 숨기는 건 C++(USTwinHUDWidget)이 함
# 위젯 트리 편집은 C++ 도우미(UTSWidgetEditLibrary), 속성은 여기서 설정. 다시 실행해도 같은 이름이면 새로 안 만듦
import unreal

WBP_PATH = "/Game/UI/WBP_TwinHUD"
L = unreal.TSWidgetEditLibrary

wbp = unreal.load_asset(WBP_PATH)

# 한글이 나오는 기존 글꼴을 그대로 쓰고 크기만 바꿈
base_font = L.find_widget(wbp, "WarriorLabel").get_editor_property("font")


def font(size):
    f = unreal.SlateFontInfo()
    f.import_text(base_font.export_text())
    f.set_editor_property("size", size)
    return f


def place(widget, x, y, w, h):
    slot = widget.get_editor_property("slot")
    slot.set_position(unreal.Vector2D(x, y))
    slot.set_size(unreal.Vector2D(w, h))


def text(parent, name, value, size, color, x, y, w, h):
    t = L.add_child_widget(wbp, parent, unreal.TextBlock, name)
    t.set_editor_property("text", value)
    t.set_editor_property("font", font(size))
    t.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    t.set_editor_property("justification", unreal.TextJustify.CENTER)
    t.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
    place(t, x, y, w, h)
    return t


# 초상 영역 (카드 기준)
PX, PY, PW, PH = 3, 3, 168, 112

for prefix in ("Warrior", "Mage"):
    card = f"{prefix}Card"

    # 쿨다운 덮개: 반투명 검정이 초상을 덮고, 남은 시간이 줄면 오른쪽부터 걷힘
    cover = L.add_child_widget(wbp, card, unreal.ProgressBar, f"{prefix}SwapCooldown")
    cover.set_editor_property("percent", 1.0)
    cover.set_editor_property("fill_color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 0.6))
    style = cover.get_editor_property("widget_style")
    background = style.get_editor_property("background_image")
    background.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(0.0, 0.0, 0.0, 0.0)))
    style.set_editor_property("background_image", background)
    cover.set_editor_property("widget_style", style)
    cover.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
    place(cover, PX, PY, PW, PH)

    # 남은 시간, 쓰러짐 (초상 가운데)
    text(card, f"{prefix}SwapCooldownText", "1.2", 26, unreal.LinearColor(1.0, 1.0, 1.0, 1.0), PX, PY + 36, PW, 40)
    text(card, f"{prefix}DownText", "쓰러짐", 20, unreal.LinearColor(0.95, 0.3, 0.3, 1.0), PX, PY + 40, PW, 32)

# 패배: 화면 정중앙
defeat = L.add_child_widget(wbp, "CanvasPanel_0", unreal.TextBlock, "DefeatText")
defeat.set_editor_property("text", "패배")
defeat.set_editor_property("font", font(48))
defeat.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)))
defeat.set_editor_property("justification", unreal.TextJustify.CENTER)
defeat.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
slot = defeat.get_editor_property("slot")
anchors = unreal.Anchors()
anchors.set_editor_property("minimum", unreal.Vector2D(0.5, 0.5))
anchors.set_editor_property("maximum", unreal.Vector2D(0.5, 0.5))
slot.set_anchors(anchors)
slot.set_alignment(unreal.Vector2D(0.5, 0.5))
slot.set_position(unreal.Vector2D(0.0, 0.0))
slot.set_auto_size(True)

L.mark_modified(wbp)
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
unreal.EditorAssetLibrary.save_loaded_asset(wbp, only_if_is_dirty=False)

# 결과: 추가된 위젯만 다시 보여 줌
tree = L.dump_widget_tree(wbp)
print("\n".join(line for line in tree.splitlines() if "Card" in line or "Swap" in line or "Down" in line or "Defeat" in line))

# ---------- 단계 6-3: 완벽 회피 글자 (화면 가운데보다 조금 위) ----------
perfect = L.add_child_widget(wbp, "CanvasPanel_0", unreal.TextBlock, "PerfectEvadeText")
perfect.set_editor_property("text", "완벽 회피!")
perfect.set_editor_property("font", font(40))
perfect.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 0.85, 0.3, 1.0)))
perfect.set_editor_property("justification", unreal.TextJustify.CENTER)
perfect.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
slot = perfect.get_editor_property("slot")
anchors = unreal.Anchors()
anchors.set_editor_property("minimum", unreal.Vector2D(0.5, 0.3))
anchors.set_editor_property("maximum", unreal.Vector2D(0.5, 0.3))
slot.set_anchors(anchors)
slot.set_alignment(unreal.Vector2D(0.5, 0.5))
slot.set_position(unreal.Vector2D(0.0, 0.0))
slot.set_auto_size(True)
L.mark_modified(wbp)
unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
unreal.EditorAssetLibrary.save_loaded_asset(wbp, only_if_is_dirty=False)
print("PerfectEvadeText 추가")
