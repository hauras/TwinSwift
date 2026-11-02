# 믹사모 막기 애니(Great Sword Blocking.fbx)를 기사 뼈대로 옮김 (IK 리타기터)
# 다른 기사 애니(Great Sword 시리즈)는 뼈 이름이 기사와 같아 바로 임포트(import_knight_anims.py)했지만,
# 이 파일은 믹사모 뼈(hips, spine1, lefthandindex4 ...)라 그대로는 안 들어감 → 메시째 임시 폴더로 가져와 리타깃
# 순서: 메시째 임포트(Mixamo/Block) → IK_Mixamo·IK_Knight → RTG_Mixamo_To_Knight → A_Knight_Block
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

SOURCE_FBX = "C:/Users/HJB/Downloads/Great Sword Blocking.fbx"
MIXAMO_DIR = "/Game/Assets/External/Mixamo/Block"
MIXAMO_MESH = f"{MIXAMO_DIR}/SK_Mixamo_Block"
MIXAMO_ANIM = f"{MIXAMO_DIR}/SK_Mixamo_Block_Anim"
KNIGHT_MESH = "/Game/Assets/External/Characters/Knight_Free/FBX/Characters/Knight_01"
KNIGHT_ANIMS = "/Game/Assets/External/Characters/Knight_Free/Animations"
RIG_DIR = "/Game/Assets/Characters/Warrior/Retarget"
RESULT = "A_Knight_Block"

# (체인 이름, 믹사모 시작·끝, 기사 시작·끝). 기사는 블렌더 rigify 뼈 (spine=골반, spine_001~003=허리·가슴, 004~005=목, 006=머리)
MIXAMO_ROOT = "hips"
KNIGHT_ROOT = "spine"
CHAINS = [
    ("Spine", "spine", "spine2", "spine_001", "spine_003"),
    ("Neck", "neck", "neck", "spine_004", "spine_005"),
    ("Head", "head", "head", "spine_006", "spine_006"),
    ("LeftClavicle", "leftshoulder", "leftshoulder", "shoulder_l", "shoulder_l"),
    ("LeftArm", "leftarm", "lefthand", "upper_arm_l", "hand_l"),
    ("RightClavicle", "rightshoulder", "rightshoulder", "shoulder_r", "shoulder_r"),
    ("RightArm", "rightarm", "righthand", "upper_arm_r", "hand_r"),
    ("LeftLeg", "leftupleg", "leftfoot", "thigh_l", "foot_l"),
    ("LeftToe", "lefttoebase", "lefttoebase", "toe_l", "toe_l"),
    ("RightLeg", "rightupleg", "rightfoot", "thigh_r", "foot_r"),
    ("RightToe", "righttoebase", "righttoebase", "toe_r", "toe_r"),
]

# ---------- 1. 믹사모 FBX를 메시째 (뼈대가 있어야 리타깃 가능) ----------
if not eal.does_asset_exist(MIXAMO_MESH):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    anim_data = ui.get_editor_property("anim_sequence_import_data")
    anim_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    anim_data.set_editor_property("import_bone_tracks", True)
    anim_data.set_editor_property("convert_scene", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE_FBX)
    task.set_editor_property("destination_path", MIXAMO_DIR)
    task.set_editor_property("destination_name", "SK_Mixamo_Block")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tools.import_asset_tasks([task])


def get_or_create(name, cls, factory):
    path = f"{RIG_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    return tools.create_asset(name, RIG_DIR, cls, factory)


def make_ik_rig(name, mesh_path, root, chain_bones):
    rig = get_or_create(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeletal_mesh(unreal.load_asset(mesh_path))
    for chain in [str(c.get_editor_property("chain_name")) for c in ctrl.get_retarget_chains()]:
        ctrl.remove_retarget_chain(chain)
    ok_root = ctrl.set_retarget_root(root)
    print(name, "골반:", ctrl.get_retarget_root(), "OK" if ok_root else "실패")
    for chain_name, start, end in chain_bones:
        added = ctrl.add_retarget_chain(chain_name, "", "", "")
        s_ok = ctrl.set_retarget_chain_start_bone(added, start)
        e_ok = ctrl.set_retarget_chain_end_bone(added, end)
        print(f"  {added}: {ctrl.get_retarget_chain_start_bone(added)} → {ctrl.get_retarget_chain_end_bone(added)}",
              "" if s_ok and e_ok else "  ← 뼈 이름 못 찾음")
    eal.save_loaded_asset(rig, only_if_is_dirty=False)
    return rig


# ---------- 2. IK 리그 ----------
mixamo_rig = make_ik_rig("IK_Mixamo", MIXAMO_MESH, MIXAMO_ROOT, [(c[0], c[1], c[2]) for c in CHAINS])
knight_rig = make_ik_rig("IK_Knight", KNIGHT_MESH, KNIGHT_ROOT, [(c[0], c[3], c[4]) for c in CHAINS])

# ---------- 3. 리타기터 ----------
rtg = get_or_create("RTG_Mixamo_To_Knight", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.remove_all_ops()
rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, mixamo_rig)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, knight_rig)
rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, mixamo_rig)
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, knight_rig)
rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)

# 기사 기본 자세를 믹사모 T자 방향에 맞춤 (리타깃 자세에만 저장, 메시는 그대로)
rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
eal.save_loaded_asset(rtg, only_if_is_dirty=False)

# ---------- 4. 애니 옮기기 → 기사 애니 폴더 ----------
created = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    [eal.find_asset_data(MIXAMO_ANIM)],
    unreal.load_asset(MIXAMO_MESH),
    unreal.load_asset(KNIGHT_MESH),
    rtg,
    "SK_Mixamo_Block_Anim",
    RESULT,
    "",
    "",
    False,
    True)
for data in created:
    src = str(data.package_name)
    dst = f"{KNIGHT_ANIMS}/{RESULT}"
    if src != dst:
        if eal.does_asset_exist(dst):
            eal.delete_asset(dst)
        eal.rename_asset(src, dst)
    anim = unreal.load_asset(dst)
    print("결과:", dst, "/ 뼈대", anim.get_editor_property("skeleton").get_name(), "/ 길이", round(anim.get_play_length(), 2))
print("RETARGET_KNIGHT_BLOCK_COMPLETE")
