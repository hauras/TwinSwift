# 쿼터뷰용 베기 이펙트: Knife_light 반달(NE_attack0X)을 칼끝이 실제로 지나간 면에 맞춰 띄움
# 칼날 궤적(NS_Trail_16)은 세로로 휘두르면 위에서 볼 때 납작해져서 잘 안 보임 → 반달 이펙트로 "벤 자리"를 보여 줌
#
# 이펙트 성질 (SceneCapture로 찍어서 확인)
#   SM_slash는 XY 평면의 반지름 100 원. 빛나는 반달이 +Y → -X → -Y 순서로 약 0.17초 동안 쓸고 지나감 (가운데 = -X, 0.15초쯤)
# 계산 (몽타주 타마다)
#   1. 공격 구간 동안 칼끝 위치를 1/60초마다 샘플링 (메시 공간)
#   2. 회전 중심 = 가슴(spine_004). 칼끝이 돌아간 방향으로 면의 법선을 구함
#      너무 세로로 선 면은 MAX_TILT까지 눕히고, 면을 칼끝이 지나간 높이로 옮김
#   3. 이펙트 Z = 법선, 이펙트 -X = 쓸고 지나간 반달의 가운데 방향, 크기 = 칼끝까지 거리 / 100
#   4. 반달 가운데가 지나가는 순간(0.15초)을 칼이 공격 구간 한가운데 있을 때에 맞춰 시작 시간을 정함
# 노티파이: 엔진 Play Niagara Effect (메시에 붙임, 소켓 없음 = 메시 기준 좌표)
# PREVIEW = True면 몽타주는 안 건드리고, 에디터 빈 곳에 자세를 잡은 기사 + 이펙트를 놓고 게임 카메라 각도로 사진을 찍음
import math

import unreal

eal = unreal.EditorAssetLibrary
anim_lib = unreal.AnimationLibrary
ev = unreal.AnimPoseEvaluationOptions()

PREVIEW = globals().get("PREVIEW", False)
PREVIEW_DIR = globals().get("PREVIEW_DIR", "")

MONTAGE_DIR = "/Game/Asset/Knight_Free/Montages"
FX_DIR = "/Game/Knife_light/VFX"
TRACK = "Slash"

# 이펙트 반달 가운데가 지나가는 시간(초)
FX_MID_TIME = 0.15
# 칼끝보다 조금 크게 (칼날이 반달 안쪽에 들어오게)
SIZE_MARGIN = 1.1
# 반달 면이 바닥에서 이 각도(도)보다 더 서지 않게 (쿼터뷰에서 세로로 선 면은 선처럼 납작하게 보임)
MAX_TILT = 40.0

# (몽타주, 애니, 이펙트, [(공격 시작, 공격 끝)]) — 구간은 칼끝 궤적으로 확인한 실제 베는 구간
# 1타 Slash_03: 0.53~0.80 오른쪽 어깨 위 → 앞 → 왼쪽 아래 대각선 내려치기
# 2타 Slash_01: 0.73~1.13 왼쪽 → 앞 → 오른쪽 가로 베기 (마무리라 금색. 몽타주 이름은 예전 그대로 Combo3)
# (Combo2 Slash_02는 콤보에서 뺌: 0.53~0.80 머리 위 칼 돌리기, 1.40~1.57 내려찍기)
STRIKES = [
    ("AM_Warrior_Combo1", "A_Knight_Slash_03", "NE_attack02", [(0.53, 0.80)]),
    ("AM_Warrior_Combo3", "A_Knight_Slash_01", "NE_attack05", [(0.73, 1.13)]),
    # 돌진 베기 SlideAttack: 1.17~1.60 미끄러짐이 끝나고 몸을 돌리며 가로로 크게 벰 (약 270도) → 반달 두 개로 나눔
    ("AM_Warrior_DashSlash", "A_Knight_SlideAttack", "NE_attack05", [(1.17, 1.40), (1.37, 1.60)]),
    # 내려찍기 JumpAttack: 0.97~1.25 머리 위에서 앞 바닥으로 세로 내려치기 (1.23초에 칼끝이 바닥)
    ("AM_Warrior_GroundSlam", "A_Knight_JumpAttack", "NE_attack02", [(0.97, 1.25)]),
]

# 판정을 몽타주(Weapon Trace)가 아니라 능력 코드가 하는 몽타주 → Hit 트랙은 만들지 않음
# (돌진 베기: USGA_DashSlash가 지나간 길을 쓸어서 판정 / 내려찍기: 착지 신호 AN_SendEvent로 USGA_GroundSlam이 범위 판정)
NO_HIT_TRACK = {"AM_Warrior_DashSlash", "AM_Warrior_GroundSlam"}

# 몽타주에 칼날 궤적이 아직 없을 때 쓰는 기본값 (setup_trail_notify.py와 같음)
DEFAULT_TRAIL = "/Game/SwordTrailVFX/VFX/NS_Trail_16"
DEFAULT_TRAIL_SOCKET = "TrailSocket"

knight = unreal.load_asset("/Game/Asset/Knight_Free/FBX/Characters/Knight_01")
weapon_socket = knight.find_socket("WeaponSocket")
w_loc = weapon_socket.get_editor_property("relative_location")
w_rot = weapon_socket.get_editor_property("relative_rotation").quaternion()
w_scale = weapon_socket.get_editor_property("relative_scale")
sword = unreal.load_asset("/Game/Asset/Knight_Free/base_basic_shaded")
tip_z = sword.find_socket("TrailEnd").get_editor_property("relative_location").z


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


def pose_at(anim, t):
    return unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, t, ev)


def tip_at(anim, t):
    hand = unreal.AnimPoseExtensions.get_bone_pose(pose_at(anim, t), "hand_R", unreal.AnimPoseSpaces.WORLD)
    return hand.transform_location(w_loc + w_rot.rotate_vector(unreal.Vector(0, 0, tip_z * w_scale.z)))


def chest_at(anim, t):
    return unreal.AnimPoseExtensions.get_bone_pose(pose_at(anim, t), "spine_004", unreal.AnimPoseSpaces.WORLD).translation


def fit_strike(anim, start, end):
    """공격 구간의 칼끝 궤적 → 이펙트 위치·회전·크기·시작 시간 (메시 공간)"""
    steps = max(2, int((end - start) * 60))
    times = [start + (end - start) * i / steps for i in range(steps + 1)]
    center = unreal.Vector(0, 0, 0)
    for t in times:
        center += chest_at(anim, t)
    center = center / len(times)
    arms = [tip_at(anim, t) - center for t in times]

    # 칼끝이 돈 방향 = 면의 법선 (연속한 두 칼끝 위치의 외적 합)
    normal = unreal.Vector(0, 0, 0)
    for a, b in zip(arms, arms[1:]):
        normal += a.cross(b)
    normal = normal.normal()

    # 너무 세로로 선 면은 바닥 쪽으로 눕힘 (도는 방향은 유지)
    up = unreal.Vector(0, 0, 1) if normal.z >= 0 else unreal.Vector(0, 0, -1)
    side = normal - up * normal.dot(up)
    if side.length() > 1e-3:
        tilt = math.degrees(math.acos(max(-1.0, min(1.0, normal.dot(up)))))
        if tilt > MAX_TILT:
            normal = (up * math.cos(math.radians(MAX_TILT)) + side.normal() * math.sin(math.radians(MAX_TILT))).normal()

    # 반달 면을 칼끝이 지나간 높이로 옮김 (가슴 높이에 깔리면 머리 위 베기보다 작고 낮아 보임)
    lift = sum(arm.dot(normal) for arm in arms) / len(arms)
    center = center + normal * lift
    arms = [arm - normal * lift for arm in arms]

    # 면에 눌러 담은 칼끝 방향들의 평균 = 반달 가운데 방향, 평균 길이 = 반지름
    middle = unreal.Vector(0, 0, 0)
    radius = 0.0
    for arm in arms:
        flat = arm - normal * arm.dot(normal)
        middle += flat.normal()
        radius += flat.length()
    middle = middle.normal()
    radius /= len(arms)

    rotation = unreal.MathLibrary.make_rot_from_xz(-middle, normal)
    scale = radius / 100.0 * SIZE_MARGIN
    spawn_time = max(0.0, (start + end) * 0.5 - FX_MID_TIME)
    return center, rotation, scale, spawn_time


results = []
for montage_name, anim_name, fx_name, windows in STRIKES:
    anim = load(f"/Game/Asset/Knight_Free/Animations/{anim_name}")
    fx = load(f"{FX_DIR}/{fx_name}")
    fits = [fit_strike(anim, s, e) for s, e in windows]
    for (s, e), (center, rotation, scale, spawn_time) in zip(windows, fits):
        print(f"{montage_name} {s:.2f}~{e:.2f}: {fx_name} {spawn_time:.2f}초, 위치 {center}, 회전 {rotation}, 크기 {scale:.2f}")
        results.append((montage_name, anim, fx, (s, e), center, rotation, scale, spawn_time))

    if PREVIEW:
        continue

    montage = load(f"{MONTAGE_DIR}/{montage_name}")
    anim_lib.remove_animation_notify_events_by_track(montage, TRACK)
    if TRACK not in [str(n) for n in anim_lib.get_animation_notify_track_names(montage)]:
        anim_lib.add_animation_notify_track(montage, TRACK)
    for center, rotation, scale, spawn_time in fits:
        notify = anim_lib.add_animation_notify_event(montage, TRACK, spawn_time, unreal.AnimNotify_PlayNiagaraEffect)
        notify.set_editor_property("template", fx)
        notify.set_editor_property("attached", True)
        notify.set_editor_property("location_offset", center)
        notify.set_editor_property("rotation_offset", rotation)
        notify.set_editor_property("scale", unreal.Vector(scale, scale, scale))

    # 판정·칼날 궤적도 실제 베는 구간으로 맞춤
    for track, cls in (("Hit", unreal.ANS_WeaponTrace), ("Trail", unreal.AnimNotifyState_TimedNiagaraEffect)):
        if track == "Hit" and montage_name in NO_HIT_TRACK:
            continue
        if track not in [str(n) for n in anim_lib.get_animation_notify_track_names(montage)]:
            anim_lib.add_animation_notify_track(montage, track)
        old = anim_lib.get_animation_notify_events_for_track(montage, track)
        trail_fx, trail_socket = load(DEFAULT_TRAIL), DEFAULT_TRAIL_SOCKET
        for event in old:
            state = event.get_editor_property("notify_state_class")
            if isinstance(state, unreal.AnimNotifyState_TimedNiagaraEffect):
                trail_fx = state.get_editor_property("template")
                trail_socket = state.get_editor_property("socket_name")
        anim_lib.remove_animation_notify_events_by_track(montage, track)
        # 칼날 궤적은 구간이 겹치면 하나로 이어 붙임 (리본이 두 번 생기지 않게)
        spans = [(min(s for s, _ in windows), max(e for _, e in windows))] if track == "Trail" else windows
        for s, e in spans:
            state = anim_lib.add_animation_notify_state_event(montage, track, s, e - s, cls)
            if track == "Trail":
                state.set_editor_property("template", trail_fx)
                state.set_editor_property("socket_name", trail_socket)
                state.set_editor_property("destroy_at_end", False)
    eal.save_loaded_asset(montage, only_if_is_dirty=False)

# ---------- 미리보기: 기사를 공격 자세로 세우고 이펙트를 그 순간까지 진행시켜 사진 ----------
if PREVIEW:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if a.get_actor_label().startswith("ZZ_FXTest"):
            a.destroy_actor()
    base = unreal.Vector(20000, 20000, 0)
    floor = eas.spawn_actor_from_class(unreal.StaticMeshActor, base + unreal.Vector(0, 900, 0))
    floor.set_actor_label("ZZ_FXTest_floor")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
    floor.set_actor_scale3d(unreal.Vector(40, 40, 1))
    view_time = globals().get("VIEW_OFFSET", 0.0)
    for i, (montage_name, anim, fx, (s, e), center, rotation, scale, spawn_time) in enumerate(results):
        # 액터 앞 = +X. 메시는 Yaw -90 (게임과 같음)
        actor_loc = base + unreal.Vector(0, i * 450, 0)
        actor_tf = unreal.Transform(actor_loc, unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1))
        mesh_tf = unreal.MathLibrary.compose_transforms(unreal.Transform(unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, -90), unreal.Vector(1, 1, 1)), actor_tf)
        t = (s + e) * 0.5 + view_time
        sk = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, mesh_tf.translation, mesh_tf.rotation.rotator())
        sk.set_actor_label(f"ZZ_FXTest_knight{i}")
        comp = sk.skeletal_mesh_component
        comp.set_skeletal_mesh_asset(knight)
        comp.set_update_animation_in_editor(True)
        comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        comp.set_animation(anim)
        comp.set_play_rate(0.0)
        comp.set_position(t, False)
        sword_actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, mesh_tf.translation)
        sword_actor.set_actor_label(f"ZZ_FXTest_sword{i}")
        sword_actor.static_mesh_component.set_static_mesh(sword)
        sword_actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        sword_actor.attach_to_component(comp, "WeaponSocket", unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET, False)
        # 칼끝이 지나간 길 (작은 공). 이펙트 반달이 이 길과 겹치면 맞은 것
        sphere = unreal.load_asset("/Engine/BasicShapes/Sphere")
        steps = 8
        for k in range(steps + 1):
            tt = s + (e - s) * k / steps
            dot = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.MathLibrary.transform_location(mesh_tf, tip_at(anim, tt)))
            dot.set_actor_label(f"ZZ_FXTest_dot{i}_{k}")
            dot.static_mesh_component.set_static_mesh(sphere)
            dot.set_actor_scale3d(unreal.Vector(0.08, 0.08, 0.08) * (1 + k * 0.15))
        fx_tf = unreal.MathLibrary.compose_transforms(unreal.Transform(center, rotation, unreal.Vector(scale, scale, scale)), mesh_tf)
        na = eas.spawn_actor_from_class(unreal.NiagaraActor, fx_tf.translation, fx_tf.rotation.rotator())
        na.set_actor_label(f"ZZ_FXTest_fx{i}")
        na.set_actor_scale3d(fx_tf.scale3d)
        nc = na.niagara_component
        nc.set_asset(fx)
        nc.activate(True)
        nc.advance_simulation(max(1, int((t - spawn_time) / 0.01)), 0.01)
        nc.set_paused(True)
    print("미리보기 배치 완료")
