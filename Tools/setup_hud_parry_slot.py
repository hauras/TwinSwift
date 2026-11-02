# WBP_TwinHUD 기술 칸 4개 → 5개 (전사 반격 자세 추가, 마법사는 5번째 칸을 코드가 숨김)
# 기존 칸 위젯은 지우지 않고 자리·크기만 다시 잡고, Skill4 위젯만 새로 만듦 (setup_twin_hud.py로 HUD를 다시 만들면 4칸으로 돌아감)
# 반격 아이콘: 아직 전용 그림이 없어 대검 콤보 아이콘(M_UI_WarriorAttack)을 "Parry" 이름으로 빌려 씀
# 다시 실행해도 됨
import unreal as u

ASSET = "/Game/UI/WBP_TwinHUD"
BACKUP = "/Game/Developers/Shared/Backups/HUD_BeforeParrySlot/WBP_TwinHUD"

# 칸 자리: 기존 (x=211+i*121, 틀 102) → 5칸 (x=207+i*100, 틀 88). 오른쪽 대기 칸(720~)과 안 겹침
START_X = 207
STEP = 100
FRAME_W = 88

GOLD = (0.4, 0.25, 0.1, 1)
NAVY = (0.018, 0.028, 0.048, 0.97)
WHITE = (0.93, 0.9, 0.82, 1)

eal = u.EditorAssetLibrary
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")
if not eal.does_asset_exist(BACKUP):
    assert eal.duplicate_asset(ASSET, BACKUP)

bp = u.load_asset(ASSET)
tree = u.find_object(None, bp.get_path_name() + ":WidgetTree")


def find(name):
    return u.find_object(None, f"{tree.get_path_name()}.{name}")


panel = find("CombatHUD")
assert panel, "CombatHUD 패널 없음"


def place(w, x, y, width, height):
    slot = w.get_editor_property("slot")
    slot.set_position(u.Vector2D(x, y))
    slot.set_size(u.Vector2D(width, height))


def new(cls, name):
    w = u.new_object(cls, outer=tree, name=name)
    panel.add_child_to_canvas(w)
    return w


def color(c):
    return u.LinearColor(*c)


def make_slot(i):
    prefix = f"Skill{i}"
    frame = new(u.Border, prefix + "Frame")
    frame.set_brush_color(color(GOLD))
    frame.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    new(u.Image, prefix + "Icon")
    cd = new(u.ProgressBar, prefix + "Cooldown")
    cd.set_fill_color_and_opacity(color((0.01, 0.015, 0.025, 0.8)))
    cd.set_percent(0)
    cd.set_editor_property("bar_fill_type", u.ProgressBarFillType.BOTTOM_TO_TOP)
    style = cd.get_editor_property("widget_style")
    background = style.get_editor_property("background_image")
    background.set_editor_property("tint_color", u.SlateColor(specified_color=u.LinearColor(0, 0, 0, 0)))
    style.set_editor_property("background_image", background)
    cd.set_editor_property("widget_style", style)
    cd.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    key_bg = new(u.Border, prefix + "KeyBG")
    key_bg.set_brush_color(color(NAVY))
    key_bg.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    for part, size, value in [("State", 16, ""), ("Key", 13, "RMB"), ("Name", 13, "반격 자세")]:
        t = new(u.TextBlock, prefix + part)
        t.set_text(value)
        f = t.get_editor_property("font")
        f.set_editor_property("size", size)
        t.set_font(f)
        t.set_color_and_opacity(u.SlateColor(specified_color=color(WHITE)))
        t.set_editor_property("justification", u.TextJustify.CENTER)
        t.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)


if not find("Skill4Frame"):
    make_slot(4)

# 5칸 모두 같은 규칙으로 자리 (원래 배치의 세로 위치·높이는 그대로)
for i in range(5):
    x = START_X + i * STEP
    p = f"Skill{i}"
    place(find(p + "Frame"), x, 48, FRAME_W, 88)
    place(find(p + "Icon"), x + 2, 50, FRAME_W - 4, 84)
    place(find(p + "Cooldown"), x + 2, 50, FRAME_W - 4, 84)
    place(find(p + "State"), x + 3, 80, FRAME_W - 6, 28)
    place(find(p + "KeyBG"), x + (FRAME_W - 60) / 2, 125, 60, 24)
    place(find(p + "Key"), x + (FRAME_W - 60) / 2, 126, 60, 22)
    place(find(p + "Name"), x - 6, 155, FRAME_W + 12, 24)

u.BlueprintEditorLibrary.compile_blueprint(bp)

# 아이콘 칸에 "Parry" 추가 (전용 아이콘이 생기면 여기만 바꿈)
cdo = u.get_default_object(bp.generated_class())
icons = dict(cdo.get_editor_property("skill_materials"))
icons[u.Name("Parry")] = u.load_asset("/Game/UI/Icons/M_UI_WarriorAttack")
cdo.set_editor_property("skill_materials", icons)
u.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)

cdo = u.get_default_object(bp.generated_class())
print("칸:", [n for n in [f"Skill{i}Frame" for i in range(5)] if find(n)])
print("아이콘:", sorted(str(k) for k in cdo.get_editor_property("skill_materials").keys()))
print("HUD_PARRY_SLOT_COMPLETE")
