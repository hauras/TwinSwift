# 골렘 공격 애니를 제자리로: 골반(hips)의 앞뒤(메시 +Y) 이동만 매 프레임 0으로. 높이·좌우·회전은 그대로
# 원본은 그대로 두고 _InPlace 복사본을 만듦 (원본은 "얼마나 언제 앞으로 가는지" 측정용)
# 골렘은 hips가 맨 위 뼈라 로컬 = 메시 공간 → 기사(make_in_place.py)처럼 루트 좌표로 바꿀 필요 없음
# 앞으로 가는 이동은 능력(USGA_BossAttack의 도약)이 캡슐을 옮겨서 대신함
import unreal

ANIMS = [
    "/Game/Assets/Characters/Boss/Golem/Animations/A_Golem_JumpAttack",
]
PELVIS = "hips"

eal = unreal.EditorAssetLibrary
lib = unreal.AnimationLibrary
AP = unreal.AnimPoseExtensions
ev = unreal.AnimPoseEvaluationOptions()


def hips_y(anim, t):
    pose = AP.get_anim_pose_at_time(anim, t, ev)
    return AP.get_bone_pose(pose, PELVIS, unreal.AnimPoseSpaces.WORLD).translation.y


for path in ANIMS:
    target = path + "_InPlace"
    if not eal.does_asset_exist(target):
        eal.duplicate_asset(path, target)
    anim = unreal.load_asset(target)

    frames = lib.get_num_frames(anim)
    start_y = lib.get_bone_pose_for_frame(anim, PELVIS, 0, False).translation.y
    before = hips_y(anim, anim.get_play_length()) - hips_y(anim, 0.0)

    positions, rotations, scales = [], [], []
    for frame in range(frames + 1):
        # 키 수는 프레임 수 + 1. 마지막 키는 마지막 프레임 값을 한 번 더
        local = lib.get_bone_pose_for_frame(anim, PELVIS, min(frame, frames - 1), False)
        loc = local.translation
        positions.append(unreal.Vector(loc.x, start_y, loc.z))
        rotations.append(local.rotation)
        scales.append(local.scale3d)

    controller = anim.controller
    controller.open_bracket(unreal.Text("골반 앞뒤 이동 제거 (제자리)"), True)
    controller.set_bone_track_keys(PELVIS, positions, rotations, scales, True)
    controller.close_bracket(True)
    eal.save_loaded_asset(anim, only_if_is_dirty=False)

    ys = [hips_y(anim, anim.get_play_length() * k / 20) - start_y for k in range(21)]
    print(f"{target.split('/')[-1]}: 앞뒤 이동 {before:.1f} → 범위 {min(ys):.1f}~{max(ys):.1f} (메시 단위)")
