# 공격 애니를 제자리로: 골반(spine)의 앞뒤(메시 +Y) 이동만 매 프레임 빼서 0으로
# 좌우 흔들림·높이 변화·회전은 그대로. 원본은 /Game/Backups/InPlace/에 복사
# (믹사모 대검 공격 애니는 In Place 옵션이 없음)
import unreal

ANIMS = [
    "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Slash_02",
    "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Slash",
    "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Roll",
    "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_SlideAttack",
    "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_JumpAttack",
    "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_SpinAttack",
]
PELVIS = "spine"
ROOT = "Knight_metarig_01"
BACKUP_DIR = "/Game/Backups/InPlace"

eal = unreal.EditorAssetLibrary
lib = unreal.AnimationLibrary
mathlib = unreal.MathLibrary
ev = unreal.AnimPoseEvaluationOptions()


def world(anim, t, bone):
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, t, ev)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)


for path in ANIMS:
    name = path.split("/")[-1]
    backup = f"{BACKUP_DIR}/{name}"
    if not eal.does_asset_exist(backup):
        eal.duplicate_asset(path, backup)
        print("백업:", backup)

    anim = unreal.load_asset(f"{path}.{name}")
    frames = lib.get_num_frames(anim)
    rate = frames / anim.get_play_length()

    start_y = world(anim, 0.0, PELVIS).translation.y
    before_y = world(anim, anim.get_play_length(), PELVIS).translation.y - start_y

    positions, rotations, scales = [], [], []
    for frame in range(frames + 1):
        t = min(frame / rate, anim.get_play_length())
        local = lib.get_bone_pose_for_frame(anim, PELVIS, frame, False)
        pelvis_w = world(anim, t, PELVIS)
        root_w = world(anim, t, ROOT)

        # 메시 기준 앞쪽(+Y)으로 나간 만큼을 루트 기준(스케일 100 포함) 좌표로 바꿔서 뺌
        drift = unreal.Vector(0.0, pelvis_w.translation.y - start_y, 0.0)
        a = mathlib.inverse_transform_location(root_w, pelvis_w.translation)
        b = mathlib.inverse_transform_location(root_w, pelvis_w.translation - drift)
        new_loc = local.translation - (a - b)

        positions.append(new_loc)
        rotations.append(local.rotation)
        scales.append(local.scale3d)

    controller = anim.controller
    controller.open_bracket(unreal.Text("골반 앞뒤 이동 제거 (제자리)"), True)
    controller.set_bone_track_keys(PELVIS, positions, rotations, scales, True)
    controller.close_bracket(True)
    eal.save_loaded_asset(anim, only_if_is_dirty=False)

    after_y = world(anim, anim.get_play_length(), PELVIS).translation.y - world(anim, 0.0, PELVIS).translation.y
    ys = [world(anim, anim.get_play_length() * k / 20, PELVIS).translation.y for k in range(21)]
    print(f"{name}: 골반 앞뒤 이동 {before_y:.1f}cm → {after_y:.1f}cm (전체 범위 {min(ys) - ys[0]:.1f}~{max(ys) - ys[0]:.1f}cm)")
