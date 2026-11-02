# 리타깃한 A_Knight_Block의 뿌리 뼈 크기 맞추기 (retarget_knight_block.py 다음에 실행)
# 문제: 기사 애니는 뿌리 뼈(Knight_metarig_01)가 100배 크기이고 그 아래 뼈는 미터 단위(블렌더). 리타깃 결과는 뿌리 크기 1,
#       골반 위치가 cm → 기사가 1/100로 작게 보임
# 해결: 뿌리 뼈는 A_Knight_Idle의 첫 프레임 값(크기 100)으로, 골반(spine)은 같은 월드 위치·방향이 되게 새 뿌리 기준으로 다시 계산
# 다른 뼈는 부모 기준이라 그대로. 원본은 /Game/Developers/Shared/Backups/KnightBlockRoot/에 복사
# 다시 실행해도 됨 (뿌리가 이미 100이면 건너뜀)
import unreal

ANIM = "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Block"
REFERENCE = "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Idle"
ROOT = "Knight_metarig_01"
PELVIS = "spine"
BACKUP = "/Game/Developers/Shared/Backups/KnightBlockRoot/A_Knight_Block"

eal = unreal.EditorAssetLibrary
lib = unreal.AnimationLibrary
math = unreal.MathLibrary

anim = unreal.load_asset(ANIM)
reference = unreal.load_asset(REFERENCE)
new_root = lib.get_bone_pose_for_frame(reference, ROOT, 0, False)

if lib.get_bone_pose_for_frame(anim, ROOT, 0, False).scale3d.x > 50.0:
    print("이미 고침 (뿌리 크기", lib.get_bone_pose_for_frame(anim, ROOT, 0, False).scale3d.x, ")")
else:
    if not eal.does_asset_exist(BACKUP):
        eal.duplicate_asset(ANIM, BACKUP)
        print("백업:", BACKUP)

    frames = lib.get_num_frames(anim)
    root_keys = ([], [], [])
    pelvis_keys = ([], [], [])
    for frame in range(frames + 1):
        old_root = lib.get_bone_pose_for_frame(anim, ROOT, frame, False)
        old_pelvis = lib.get_bone_pose_for_frame(anim, PELVIS, frame, False)

        # 골반의 컴포넌트 기준 위치·방향은 그대로 두고, 부모(뿌리)만 바꿈
        pelvis_world = math.compose_transforms(old_pelvis, old_root)
        location = math.inverse_transform_location(new_root, pelvis_world.translation)
        rotation = math.inverse_transform_rotation(new_root, pelvis_world.rotation.rotator()).quaternion()

        root_keys[0].append(new_root.translation)
        root_keys[1].append(new_root.rotation)
        root_keys[2].append(new_root.scale3d)
        pelvis_keys[0].append(location)
        pelvis_keys[1].append(rotation)
        pelvis_keys[2].append(old_pelvis.scale3d)

    controller = anim.controller
    controller.open_bracket(unreal.Text("뿌리 뼈 크기 100으로 (기사 애니와 같게)"), True)
    controller.set_bone_track_keys(ROOT, *root_keys, True)
    controller.set_bone_track_keys(PELVIS, *pelvis_keys, True)
    controller.close_bracket(True)
    eal.save_loaded_asset(anim, only_if_is_dirty=False)

for name, a in [("고친 막기", anim), ("기준 Idle", reference)]:
    r = lib.get_bone_pose_for_frame(a, ROOT, 0, False)
    p = lib.get_bone_pose_for_frame(a, PELVIS, 0, False)
    print(f"{name}: 뿌리 크기 {r.scale3d.x:.1f}, 골반 위치 ({p.translation.x:.2f}, {p.translation.y:.2f}, {p.translation.z:.2f})")
print("FIX_KNIGHT_BLOCK_ROOT_COMPLETE")
