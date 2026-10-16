# 칼 궤적: SwordTrailVFX 데모(BP_Man)와 같은 방식
#   데모: 칼(SM_Sword)을 스켈레톤 Sword 소켓에 붙이고, 애니에 엔진 노티파이 Timed Niagara Effect(NS_Trail_16, 같은 Sword 소켓)
#   데모 칼은 피벗 = 손잡이, +Z = 칼끝, 손잡이~칼끝 118cm. 이펙트는 이 칼에 맞춰 만들어져 있음
# 우리: hand_R에 TrailSocket을 만들고 "데모 칼이 놓인 것처럼" 맞춤
#   위치 = 우리 칼 손잡이, 방향 = 우리 칼과 같음(가드 방향만 데모처럼 90도 돌림), 크기 = 우리 칼 손잡이~칼끝 / 118
#   그리고 콤보 몽타주의 판정 구간(Weapon Trace)마다 같은 구간에 Timed Niagara Effect를 깜
# 조건: hand_R 아래에 TrailSocket 소켓이 있어야 함 (없으면 만들라는 안내가 나옴. Python으로는 소켓 이름을 못 정함)
import unreal

eal = unreal.EditorAssetLibrary
anim_lib = unreal.AnimationLibrary

TRAIL = "/Game/Assets/External/VFX/SwordTrailVFX/VFX/NS_Trail_16.NS_Trail_16"
SOCKET = "TrailSocket"
TRACK = "Trail"

# 기사 루트 뼈 스케일 100 → 소켓 값은 /100
BONE_SCALE = 100.0
# 데모 칼: 피벗(손잡이)~칼끝
DEMO_GRIP_TO_TIP = 118.3
# 우리 칼: 폼멜에서 손잡이까지 (setup_weapon.py의 GRIP)
GRIP = 32.0

MONTAGES = [
    "/Game/Assets/Characters/Warrior/Montages/AM_Warrior_Combo1",
    "/Game/Assets/Characters/Warrior/Montages/AM_Warrior_Combo3",
]

# 판정(Hit) 구간은 setup_slash_fx.py가 칼끝 궤적으로 정함. 여기서는 그 구간에 칼날 궤적만 깜


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 1. TrailSocket: 칼 소켓(WeaponSocket) 기준으로 계산 ----------
knight = load("/Game/Assets/External/Characters/Knight_Free/FBX/Characters/Knight_01")
weapon_socket = knight.find_socket("WeaponSocket")
trail_socket = knight.find_socket(SOCKET)
if not trail_socket:
    raise RuntimeError(f"{SOCKET} 소켓이 없음: 스켈레톤 에디터에서 hand_R 우클릭 → Add Socket → 이름 {SOCKET}")

sword = load("/Game/Assets/Characters/Warrior/base_basic_shaded")
tip = sword.get_bounding_box().max.z
scale = (tip - GRIP) / DEMO_GRIP_TO_TIP

w_loc = weapon_socket.get_editor_property("relative_location")
w_rot = weapon_socket.get_editor_property("relative_rotation")
w_scale = weapon_socket.get_editor_property("relative_scale")

# 칼 메시 안의 손잡이 위치(0,0,GRIP)를 손 기준으로 바꿈 (칼 소켓의 크기·회전 적용)
grip_in_hand = w_loc + w_rot.quaternion().rotate_vector(unreal.Vector(0, 0, GRIP * w_scale.z))

# 데모 칼은 가드가 Y, 우리 칼은 가드가 X → 칼날 축(Z)으로 90도 돌려 데모와 같은 놓임으로
rotation = (w_rot.quaternion() * unreal.Rotator(0, 0, 90).quaternion()).rotator()

if str(trail_socket.get_editor_property("bone_name")) != "hand_R":
    raise RuntimeError(f"{SOCKET}이 hand_R 아래가 아님")
trail_socket.set_editor_property("relative_location", grip_in_hand)
trail_socket.set_editor_property("relative_rotation", rotation)
trail_socket.set_editor_property("relative_scale", unreal.Vector(scale, scale, scale) / BONE_SCALE)
eal.save_loaded_asset(knight, only_if_is_dirty=False)
eal.save_loaded_asset(knight.skeleton, only_if_is_dirty=False)
print(f"{SOCKET}: 위치 {grip_in_hand}, 회전 {rotation}, 크기 {scale:.3f}배 (칼 손잡이~칼끝 {tip - GRIP:.1f}cm / 데모 {DEMO_GRIP_TO_TIP})")

# ---------- 2. 몽타주: Weapon Trace 구간마다 Timed Niagara Effect ----------
trail_system = unreal.load_asset(TRAIL)
for path in MONTAGES:
    montage = load(path)

    # 판정 구간 읽기 (Hit 트랙의 Weapon Trace)
    windows = []
    for event in anim_lib.get_animation_notify_events_for_track(montage, "Hit"):
        state = event.get_editor_property("notify_state_class")
        if isinstance(state, unreal.ANS_WeaponTrace):
            start = anim_lib.get_anim_notify_event_trigger_time(event)
            duration = anim_lib.get_anim_notify_event_duration(event)
            windows.append((start, duration))

    name = path.split("/")[-1]

    # 다시 실행해도 겹치지 않게 Trail 트랙을 비우고 다시 깜
    anim_lib.remove_animation_notify_events_by_track(montage, TRACK)
    if TRACK not in [str(n) for n in anim_lib.get_animation_notify_track_names(montage)]:
        anim_lib.add_animation_notify_track(montage, TRACK)
    for start, duration in windows:
        state = anim_lib.add_animation_notify_state_event(
            montage, TRACK, start, duration, unreal.AnimNotifyState_TimedNiagaraEffect)
        state.set_editor_property("template", trail_system)
        state.set_editor_property("socket_name", SOCKET)
        # 끝나면 끄기만 → 남은 리본은 이펙트 수명대로 사라짐 (데모와 같음)
        state.set_editor_property("destroy_at_end", False)
    eal.save_loaded_asset(montage, only_if_is_dirty=False)
    print(name, "판정·궤적:", ", ".join(f"{s:.2f}~{s + d:.2f}초" for s, d in windows))
