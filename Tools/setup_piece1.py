# 조각 1 에셋 만들기 (입력, 카메라 설정, 컨트롤러·게임모드 BP, 테스트 맵)
# 실행: 엔진 python -I -X utf8 Tools/ue_exec.py Tools/setup_piece1.py
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def make(name, path, cls, factory):
    full = f"{path}/{name}"
    if eal.does_asset_exist(full):
        print("이미 있음:", full)
        return eal.load_asset(full)
    asset = asset_tools.create_asset(name, path, cls, factory)
    print("만듦:", full)
    return asset


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


# ---------- 입력 ----------
def make_action(name, value_type):
    ia = make(name, "/Game/Input", unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", value_type)
    return ia


ia_move = make_action("IA_Move", unreal.InputActionValueType.AXIS2D)
ia_look = make_action("IA_Look", unreal.InputActionValueType.AXIS2D)
ia_walk = make_action("IA_Walk", unreal.InputActionValueType.BOOLEAN)
ia_cam = make_action("IA_CycleCamera", unreal.InputActionValueType.BOOLEAN)

imc = make("IMC_Default", "/Game/Input", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()


def swizzle(outer):
    # 키 입력(X축)을 Y축(앞뒤)으로 옮김
    return unreal.new_object(unreal.InputModifierSwizzleAxis, outer)


def negate(outer):
    # 모든 축 반전 (기본값). bX 같은 속성은 Python에서 못 바꿔서 기본값만 씀
    return unreal.new_object(unreal.InputModifierNegate, outer)


# (액션, 키, 모디파이어 만드는 함수)
bindings = [
    (ia_move, "W", lambda o: [swizzle(o)]),
    (ia_move, "S", lambda o: [swizzle(o), negate(o)]),
    (ia_move, "A", lambda o: [negate(o)]),
    (ia_move, "D", lambda o: []),
    # 마우스 X·Y를 따로 매핑: Y는 앞뒤 축으로 옮기고 반전 (위로 밀면 위를 봄)
    (ia_look, "MouseX", lambda o: []),
    (ia_look, "MouseY", lambda o: [swizzle(o), negate(o)]),
    (ia_walk, "LeftShift", lambda o: []),
    (ia_cam, "V", lambda o: []),
]
for action, key_name, _ in bindings:
    imc.map_key(action, key(key_name))

# map_key가 돌려준 값은 복사본이라, 배열을 통째로 꺼내 고친 뒤 다시 넣음
data = imc.get_editor_property("default_key_mappings")
mappings = list(data.get_editor_property("mappings"))
for i, (_, key_name, make_mods) in enumerate(bindings):
    mappings[i].set_editor_property("modifiers", make_mods(imc))
data.set_editor_property("mappings", mappings)
imc.set_editor_property("default_key_mappings", data)

# ---------- 카메라 설정 ----------
def make_camera(name, **props):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.SCameraConfig)
    da = make(name, "/Game/Camera", unreal.SCameraConfig, factory)
    for k, v in props.items():
        da.set_editor_property(k, v)
    return da


cam_quarter = make_camera(
    "DA_Camera_Quarter",
    arm_length=1400.0,
    socket_offset=unreal.Vector(0, 0, 0),
    field_of_view=50.0,
    free_look=False,
    fixed_rotation=unreal.Rotator(roll=0, pitch=-55, yaw=45),
    enable_camera_lag=True,
    camera_lag_speed=8.0,
    aim_mode=unreal.SAimMode.CURSOR,
)

cam_third = make_camera(
    "DA_Camera_ThirdPerson",
    arm_length=600.0,
    socket_offset=unreal.Vector(0, 0, 150),
    field_of_view=90.0,
    free_look=True,
    min_pitch=-60.0,
    max_pitch=10.0,
    enable_camera_lag=True,
    camera_lag_speed=10.0,
    aim_mode=unreal.SAimMode.SCREEN_CENTER,
)

# ---------- 블루프린트 ----------
def make_bp(name, parent):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    return make(name, "/Game/Blueprints", unreal.Blueprint, factory)


bp_pc = make_bp("BP_PlayerController", unreal.SPlayerController)
pc_cdo = unreal.get_default_object(bp_pc.generated_class())
pc_cdo.set_editor_property("camera_configs", [cam_quarter, cam_third])
pc_cdo.set_editor_property("default_mapping_context", imc)
pc_cdo.set_editor_property("move_action", ia_move)
pc_cdo.set_editor_property("look_action", ia_look)
pc_cdo.set_editor_property("walk_action", ia_walk)
pc_cdo.set_editor_property("cycle_camera_action", ia_cam)

bp_gm = make_bp("BP_GameMode", unreal.SGameMode)
gm_cdo = unreal.get_default_object(bp_gm.generated_class())
gm_cdo.set_editor_property("player_controller_class", bp_pc.generated_class())

for a in [ia_move, ia_look, ia_walk, ia_cam, imc, cam_quarter, cam_third, bp_pc, bp_gm]:
    if isinstance(a, unreal.Blueprint):
        unreal.BlueprintEditorLibrary.compile_blueprint(a)
    eal.save_loaded_asset(a, only_if_is_dirty=False)

print("완료: 입력·카메라·BP")
