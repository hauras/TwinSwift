# BP_Player의 WeaponMesh(C++ 컴포넌트)에 대검 지정 + 손 안 위치 맞추기
# 위치·각도는 Idle·Walk·Run의 두 손 위치로 계산 (오른손이 가드 쪽, 칼날은 왼손→오른손 방향으로 뻗음)
# 조건: WeaponMesh가 들어간 C++이 빌드돼 있어야 함
import unreal

eal = unreal.EditorAssetLibrary

# 주의: 기사 루트 뼈에 스케일 100이 들어 있어서 붙이는 물체도 100배가 됨 → 값을 100으로 나눔
# (Blender에서 아마추어 스케일을 적용하고 다시 내보내면 0.6 / (-17.4, -5.9, 5.7)로 되돌릴 것)
BONE_SCALE = 100.0

# 기사 키 약 210cm(깃털 포함). 검 길이는 원본 그대로 190cm(키의 90%)
# 2~3등신 저폴리라 칼날 폭·두께는 1.5배로 두툼하게 (얇으면 단검처럼 보임)
LENGTH = 1.0
THICK = 1.5

# 오른손이 잡는 곳: 폼멜에서 32cm (가드 바로 아래 손잡이)
GRIP = 32.0

# Idle·Walk·Run 두 손 위치로 계산한 손 기준 칼날 방향 (단위 벡터)
BLADE_DIR = unreal.Vector(0.906, 0.306, -0.297)

bp = unreal.load_asset("/Game/Blueprints/BP_Player.BP_Player")
cdo = unreal.get_default_object(bp.generated_class())
weapon = cdo.get_editor_property("weapon_mesh")
weapon.set_static_mesh(unreal.load_asset("/Game/Asset/Knight_Free/base_basic_shaded"))
weapon.set_editor_property("relative_location", BLADE_DIR * (-GRIP * LENGTH / BONE_SCALE))
weapon.set_editor_property("relative_rotation", unreal.Rotator(roll=-107.2, pitch=0.0, yaw=108.6))
weapon.set_editor_property("relative_scale3d", unreal.Vector(THICK, THICK, LENGTH) / BONE_SCALE)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)

print("WeaponMesh:", weapon.static_mesh.get_name(), "붙은 곳:", weapon.get_attach_socket_name())
