# 보스 등장신 배치
# 1) WBP_BossHUD에 이름 카드 IntroTitleText (화면 가운데 위쪽 큰 글자)
# 2) 레벨: 전투 진행 액터(경기장 가운데)를 기준으로
#    - 골렘: 플레이어 반대쪽 가장자리 (가운데에서 GOLEM_EDGE만큼) → 등장할 때 가운데로 뛰어내림
#    - PlayerStart: 진입 상자 밖 (걸어 들어가야 시작)
#    - 등장 카메라: 플레이어 쪽 높은 곳에서 가운데를 내려다봄
# APPLY = False면 계산한 위치만 출력 (레벨을 바꾸기 전에 확인)
import math
import unreal

APPLY = False

GOLEM_EDGE = 1200.0       # 가운데에서 골렘까지 (진입 상자 반경 1500 안쪽)
PLAYER_OUTSIDE = 300.0    # 진입 상자 바깥으로 얼마나 더
CAMERA_BACK = 1500.0      # 가운데에서 플레이어 쪽으로
CAMERA_HEIGHT = 900.0

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()

encounter = next(a for a in all_actors if isinstance(a, unreal.SBossEncounter))
boss = encounter.get_editor_property("boss")
player_start = next(a for a in all_actors if isinstance(a, unreal.PlayerStart))
trigger = encounter.get_editor_property("trigger")
extent = trigger.get_scaled_box_extent()

center = encounter.get_actor_location()
ps = player_start.get_actor_location()

# 가운데 → 플레이어 시작 방향 (바닥 평면)
to_player = unreal.Vector(ps.x - center.x, ps.y - center.y, 0.0)
if to_player.length() < 1.0:
    to_player = unreal.Vector(-1.0, 0.0, 0.0)
to_player = to_player.normal()

# 상자 가장자리까지 거리 (그 방향으로) + 바깥 여유
edge = min(extent.x / max(abs(to_player.x), 1e-3), extent.y / max(abs(to_player.y), 1e-3))
new_ps = unreal.Vector(center.x + to_player.x * (edge + PLAYER_OUTSIDE), center.y + to_player.y * (edge + PLAYER_OUTSIDE), ps.z)

boss_loc = boss.get_actor_location()
new_boss = unreal.Vector(center.x - to_player.x * GOLEM_EDGE, center.y - to_player.y * GOLEM_EDGE, boss_loc.z)
boss_yaw = math.degrees(math.atan2(to_player.y, to_player.x))  # 가운데(플레이어 쪽)를 봄

cam = unreal.Vector(center.x + to_player.x * CAMERA_BACK, center.y + to_player.y * CAMERA_BACK, boss_loc.z + CAMERA_HEIGHT)
look = unreal.Vector(center.x - cam.x, center.y - cam.y, boss_loc.z - cam.z)
cam_yaw = math.degrees(math.atan2(look.y, look.x))
cam_pitch = math.degrees(math.atan2(look.z, math.hypot(look.x, look.y)))

print(f"경기장 가운데 {center}, 상자 반경 {extent.x:.0f}x{extent.y:.0f}")
print(f"PlayerStart {ps} → {new_ps}")
print(f"골렘 {boss_loc} → {new_boss}, 방향 yaw {boss_yaw:.0f}")
print(f"등장 카메라 {cam}, yaw {cam_yaw:.0f}, pitch {cam_pitch:.0f}")

if not APPLY:
    raise SystemExit("APPLY = False: 위치만 출력함")

# ---------- 레벨 ----------
player_start.set_actor_location(new_ps, False, False)
player_start.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=boss_yaw + 180.0), False)
boss.set_actor_location(new_boss, False, False)
boss.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=boss_yaw), False)

camera = next((a for a in all_actors if isinstance(a, unreal.CameraActor) and a.get_actor_label() == "IntroCamera"), None)
if not camera:
    camera = actors.spawn_actor_from_class(unreal.CameraActor, cam)
    camera.set_actor_label("IntroCamera")
camera.set_actor_location(cam, False, False)
camera.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=cam_pitch, yaw=cam_yaw), False)
encounter.set_editor_property("intro_camera", camera)

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("레벨 저장")

# ---------- 이름 카드 ----------
lib = unreal.TSWidgetEditLibrary
hud = unreal.load_asset("/Game/UI/WBP_BossHUD")
root = lib.ensure_root_canvas(hud, "Root")
base_font = lib.find_widget(hud, "BossNameText").get_editor_property("font")
font = unreal.SlateFontInfo()
font.import_text(base_font.export_text())
font.set_editor_property("size", 72)

title = lib.add_child_widget(hud, root, unreal.TextBlock, "IntroTitleText")
title.set_editor_property("text", unreal.Text("바위 골렘"))
title.set_editor_property("font", font)
title.set_editor_property("justification", unreal.TextJustify.CENTER)
title.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(1.0, 0.85, 0.55, 1.0)))
title.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
slot = title.get_editor_property("slot")
slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.5, 0.28), maximum=unreal.Vector2D(0.5, 0.28)))
slot.set_alignment(unreal.Vector2D(0.5, 0.5))
slot.set_auto_size(True)

lib.mark_modified(hud)
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
eal.save_loaded_asset(hud)
print([l for l in lib.dump_widget_tree(hud).splitlines() if "Intro" in l])
