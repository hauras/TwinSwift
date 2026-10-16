# 대검 위치·각도·크기를 스켈레톤의 WeaponSocket(hand_R 아래)에 넣고, BP_Player의 WeaponMesh는 소켓에 그대로 붙게(0) 함
# 게임 칼(C++ WeaponMesh)과 애니 에디터 미리보기 칼이 같은 소켓을 써서 똑같이 보임
# 조건: 스켈레톤 에디터에서 hand_R 우클릭 → Add Socket → 이름 WeaponSocket (Python으로는 소켓 이름을 정할 수 없음)
# 칼 방향은 Idle 자세에서 "칼끝이 캐릭터 정면 쪽 위 TIP_UP_DEG도"가 되게 오른손 기준으로 계산
# (예전: 두 손을 잇는 선 → 2~3등신이라 두 손이 옆으로 나란해서 칼이 옆으로 누움)
# 각도를 바꾸려면 아래 숫자만 고치고 다시 실행 (또는 스켈레톤 에디터에서 소켓을 직접 끌어도 됨)
import math
import unreal

eal = unreal.EditorAssetLibrary

# 주의: 기사 루트 뼈에 스케일 100이 들어 있어서 붙이는 물체도 100배가 됨 → 값을 100으로 나눔
# (Blender에서 아마추어 스케일을 적용하고 다시 내보내면 1로)
BONE_SCALE = 100.0

# 기사 키 약 210cm(깃털 포함). 검 길이는 원본 그대로 190cm(키의 90%)
# 2~3등신 저폴리라 칼날 폭·두께는 1.5배로 두툼하게 (얇으면 단검처럼 보임)
LENGTH = 1.0
THICK = 1.5

# 오른손이 잡는 곳: 폼멜에서 32cm (가드 바로 아래 손잡이)
GRIP = 32.0

# Idle에서 칼끝 방향: 정면 기준 위로 몇 도, 바깥(오른쪽)으로 몇 도
TIP_UP_DEG = 35.0
TIP_OUT_DEG = 10.0

# 메시 공간: 앞 = +Y, 위 = +Z, 오른쪽 = -X (메시가 Yaw -90으로 붙어 있음)
FORWARD = unreal.Vector(0, 1, 0)
UP = unreal.Vector(0, 0, 1)
RIGHT = unreal.Vector(-1, 0, 0)

ev = unreal.AnimPoseEvaluationOptions()
idle = unreal.load_asset("/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_Idle.A_Knight_Idle")
pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(idle, 0.0, ev)
hand = unreal.AnimPoseExtensions.get_bone_pose(pose, "hand_R", unreal.AnimPoseSpaces.WORLD)

up = math.radians(TIP_UP_DEG)
out = math.radians(TIP_OUT_DEG)
tip_dir = (FORWARD * (math.cos(up) * math.cos(out)) + RIGHT * (math.cos(up) * math.sin(out)) + UP * math.sin(up)).normal()

# 손 기준 방향으로 바꿈. 칼 메시는 +Z가 칼끝, +X가 가드 폭 → 가드는 좌우로 눕게
z_local = hand.rotation.unrotate_vector(tip_dir)
x_local = hand.rotation.unrotate_vector(RIGHT)
rotation = unreal.MathLibrary.make_rot_from_zx(z_local, x_local)

# 손잡이(폼멜에서 GRIP) 위치가 손에 오도록 피벗을 칼끝 반대 방향으로 뺌
location = z_local * (-GRIP * LENGTH / BONE_SCALE)

# ---------- 1. 스켈레톤 소켓 ----------
knight = unreal.load_asset("/Game/Assets/External/Characters/Knight_Free/FBX/Characters/Knight_01.Knight_01")
socket = knight.find_socket("WeaponSocket")
if not socket:
    raise RuntimeError("WeaponSocket 소켓이 없음: 스켈레톤 에디터에서 hand_R 우클릭 → Add Socket → 이름 WeaponSocket")
if str(socket.get_editor_property("bone_name")) != "hand_R":
    print("주의: WeaponSocket이 hand_R 아래가 아님 →", socket.get_editor_property("bone_name"))
socket.set_editor_property("relative_location", location)
socket.set_editor_property("relative_rotation", rotation)
socket.set_editor_property("relative_scale", unreal.Vector(THICK, THICK, LENGTH) / BONE_SCALE)
eal.save_loaded_asset(knight, only_if_is_dirty=False)
eal.save_loaded_asset(knight.skeleton, only_if_is_dirty=False)

# ---------- 2. BP_Player: 칼 메시만 지정, 위치는 소켓 그대로 ----------
bp = unreal.load_asset("/Game/Blueprints/BP_Player.BP_Player")
cdo = unreal.get_default_object(bp.generated_class())
weapon = cdo.get_editor_property("weapon_mesh")
weapon.set_static_mesh(unreal.load_asset("/Game/Assets/Characters/Warrior/base_basic_shaded"))
weapon.set_editor_property("relative_location", unreal.Vector(0, 0, 0))
weapon.set_editor_property("relative_rotation", unreal.Rotator(0, 0, 0))
weapon.set_editor_property("relative_scale3d", unreal.Vector(1, 1, 1))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)

print("WeaponSocket:", socket.get_editor_property("bone_name"), location, rotation, socket.get_editor_property("relative_scale"))
print("WeaponMesh:", weapon.static_mesh.get_name(), "붙은 곳:", weapon.get_attach_socket_name(), "(빌드 전이면 hand_R로 보임)")
print("Idle 칼끝 방향(메시 공간):", tip_dir)
