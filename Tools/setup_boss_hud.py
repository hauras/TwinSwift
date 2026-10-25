# 보스 HUD WBP_BossHUD 만들기 + 연결
# - 부모 USBossHUDWidget (로직은 C++). 여기서는 배치만: 위쪽 가운데 BossPanel(이름 + 체력바), 가운데 ResultText
# - 글꼴은 플레이어 HUD(WBP_TwinHUD)의 글자 글꼴을 그대로 (한글이 나오는 글꼴)
# - BP_BossEncounter의 BossHUDClass, BP_Boss_Golem의 DisplayName("바위 골렘")
# 다시 실행해도 됨 (있는 위젯은 값만 다시 넣음)
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
lib = unreal.TSWidgetEditLibrary
HUD_PATH = "/Game/UI/WBP_BossHUD"
TWIN_HUD = "/Game/UI/WBP_TwinHUD"
BOSS_BP = "/Game/Blueprints/Boss/BP_Boss_Golem"
PARENT = unreal.load_class(None, "/Script/TwinSwift.SBossHUDWidget")

# ---------- WBP 만들기 ----------
if eal.does_asset_exist(HUD_PATH):
    hud = unreal.load_asset(HUD_PATH)
else:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", PARENT)
    hud = unreal.AssetToolsHelpers.get_asset_tools().create_asset("WBP_BossHUD", "/Game/UI", unreal.WidgetBlueprint, factory)

root = lib.ensure_root_canvas(hud, "Root")

# 플레이어 HUD 글꼴 (한글)
twin = unreal.load_asset(TWIN_HUD)
base_font = lib.find_widget(twin, "PerfectEvadeText").get_editor_property("font")


def font(size):
    f = unreal.SlateFontInfo()
    f.import_text(base_font.export_text())
    f.set_editor_property("size", size)
    return f


# 위쪽 가운데: 이름 + 체력바를 세로로
panel = lib.add_child_widget(hud, root, unreal.VerticalBox, "BossPanel")
slot = panel.get_editor_property("slot")
slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.5, 0.0), maximum=unreal.Vector2D(0.5, 0.0)))
slot.set_alignment(unreal.Vector2D(0.5, 0.0))
slot.set_position(unreal.Vector2D(0.0, 40.0))
slot.set_size(unreal.Vector2D(760.0, 80.0))

name_text = lib.add_child_widget(hud, "BossPanel", unreal.TextBlock, "BossNameText")
name_text.set_editor_property("text", unreal.Text("바위 골렘"))
name_text.set_editor_property("font", font(26))
name_text.set_editor_property("justification", unreal.TextJustify.CENTER)
name_text.get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

bar = lib.add_child_widget(hud, "BossPanel", unreal.ProgressBar, "BossHealthBar")
bar.set_editor_property("percent", 1.0)
# 바위색 쪽 붉은 갈색 (플레이어 체력바와 구분)
bar.set_editor_property("fill_color_and_opacity", unreal.LinearColor(0.75, 0.18, 0.08, 1.0))
bar_slot = bar.get_editor_property("slot")
bar_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
bar_slot.set_padding(unreal.Margin(0.0, 6.0, 0.0, 0.0))
bar.set_editor_property("bar_fill_style", unreal.ProgressBarFillStyle.SCALE)
panel.set_visibility(unreal.SlateVisibility.COLLAPSED)

# 가운데: 결과 글자
result = lib.add_child_widget(hud, root, unreal.TextBlock, "ResultText")
result.set_editor_property("text", unreal.Text("클리어 0.0초 · R: 다시 시작"))
result.set_editor_property("font", font(44))
result.set_editor_property("justification", unreal.TextJustify.CENTER)
result.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 0.92, 0.7, 1.0)))
r_slot = result.get_editor_property("slot")
r_slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.5, 0.4), maximum=unreal.Vector2D(0.5, 0.4)))
r_slot.set_alignment(unreal.Vector2D(0.5, 0.5))
r_slot.set_auto_size(True)
result.set_visibility(unreal.SlateVisibility.COLLAPSED)

lib.mark_modified(hud)
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
assert eal.save_loaded_asset(hud, only_if_is_dirty=False)
print(lib.dump_widget_tree(hud))

# ---------- 연결 ----------
# 전투 진행 BP: 이름으로 찾음 (만든 폴더가 정해져 있지 않아서)
ar = unreal.AssetRegistryHelpers.get_asset_registry()
encounter_bps = [d for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Blueprint"))
                 if str(d.asset_name) in ["BP_Encounter", "BP_BossEncounter"]]
for data in encounter_bps:
    bp = unreal.load_asset(str(data.package_name))
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("boss_hud_class", hud.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    print("BossHUDClass 연결:", data.package_name)
assert encounter_bps, "Encounter BP not found"

boss = unreal.load_asset(BOSS_BP)
boss_cdo = unreal.get_default_object(boss.generated_class())
boss_cdo.set_editor_property("display_name", unreal.Text("바위 골렘"))
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
eal.save_loaded_asset(boss)
print("보스 이름:", boss_cdo.get_editor_property("display_name"))

print("BOSS_HUD_SETUP_COMPLETE")
