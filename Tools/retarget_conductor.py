# 탄막 지휘자 애니: 마법사 애니(믹사모 Pro Magic Pack)를 지휘자 뼈대로 옮김 (IK 리타기터)
# - 지휘자 뼈 16개: 몸통·머리·팔·손·로브. 다리 없음 → 마법사 다리는 옮기지 않음 (로브는 골반을 따라감)
# - 체인 이름을 양쪽 같게 지어 자동 연결 (Spine/Neck/Head/Clavicle/Arm)
# - 지휘자 팔은 A자(아래로 약 40도), 마법사는 T자 → 지휘자 리타깃 자세를 마법사 방향에 자동 정렬
# 다시 실행해도 됨: IK 리그·리타기터는 다시 설정, 애니는 덮어씀
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# 메시가 없으면 일괄 리타깃이 에디터를 멈춤(assert) → 미리 확인
for path in ("/Game/Assets/External/Characters/Wizard_Free/FBX/Characters/Air_Wizard", "/Game/Assets/Characters/Boss/Conductor/Mesh/SK_Conductor"):
    if not eal.does_asset_exist(path):
        raise RuntimeError(f"메시 없음: {path} (import_conductor.py 먼저)")

WIZARD_MESH = "/Game/Assets/External/Characters/Wizard_Free/FBX/Characters/Air_Wizard"
WIZARD_ANIMS = "/Game/Assets/External/Characters/Wizard_Free/Animations"
CONDUCTOR_MESH = "/Game/Assets/Characters/Boss/Conductor/Mesh/SK_Conductor"
RIG_DIR = "/Game/Assets/Characters/Boss/Conductor/Retarget"
ANIM_DIR = "/Game/Assets/Characters/Boss/Conductor/Animations"

# (체인 이름, 마법사 시작·끝, 지휘자 시작·끝). 블렌더 이름의 "."은 임포트하면 "_"가 될 수 있어 둘 다 시도
WIZARD_ROOT = "spine"
CONDUCTOR_ROOT = "spine"
CHAINS = [
    ("Spine", "spine.001", "spine.003", "chest", "chest"),
    ("Neck", "spine.004", "spine.005", "neck", "neck"),
    ("Head", "spine.006", "spine.006", "head", "head"),
    ("LeftClavicle", "shoulder.L", "shoulder.L", "clavicle_L", "clavicle_L"),
    ("LeftArm", "upper_arm.L", "hand.L", "upperarm_L", "hand_L"),
    ("RightClavicle", "shoulder.R", "shoulder.R", "clavicle_R", "clavicle_R"),
    ("RightArm", "upper_arm.R", "hand.R", "upperarm_R", "hand_R"),
]


def get_or_create(name, cls, factory):
    path = f"{RIG_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    return tools.create_asset(name, RIG_DIR, cls, factory)


def name_variants(bone):
    return [bone, bone.replace(".", "_")]


def make_ik_rig(name, mesh_path, root, chain_bones):
    rig = get_or_create(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeletal_mesh(unreal.load_asset(mesh_path))
    for chain in [str(c.get_editor_property("chain_name")) for c in ctrl.get_retarget_chains()]:
        ctrl.remove_retarget_chain(chain)

    ok_root = any(ctrl.set_retarget_root(n) for n in name_variants(root))
    print(name, "골반(리타깃 루트):", ctrl.get_retarget_root(), "OK" if ok_root else "실패")

    for chain_name, start, end in chain_bones:
        added = ctrl.add_retarget_chain(chain_name, "", "", "")
        s_ok = any(ctrl.set_retarget_chain_start_bone(added, n) for n in name_variants(start))
        e_ok = any(ctrl.set_retarget_chain_end_bone(added, n) for n in name_variants(end))
        print(f"  {added}: {ctrl.get_retarget_chain_start_bone(added)} → {ctrl.get_retarget_chain_end_bone(added)}",
              "" if s_ok and e_ok else "  ← 뼈 이름 못 찾음")
    eal.save_loaded_asset(rig, only_if_is_dirty=False)
    return rig


wizard_rig = make_ik_rig("IK_Wizard", WIZARD_MESH, WIZARD_ROOT, [(c[0], c[1], c[2]) for c in CHAINS])
conductor_rig = make_ik_rig("IK_Conductor", CONDUCTOR_MESH, CONDUCTOR_ROOT, [(c[0], c[3], c[4]) for c in CHAINS])

# ---------- 리타기터 ----------
rtg = get_or_create("RTG_Wizard_To_Conductor", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.remove_all_ops()
rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, wizard_rig)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, conductor_rig)
rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, wizard_rig)
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, conductor_rig)
rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)

# 지휘자 A자 팔 → 마법사 T자 방향으로 (리타깃 자세에만 저장, 메시는 그대로)
rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
print("연산:", [str(rc.get_op_name(i)) for i in range(rc.get_num_retarget_ops())])
eal.save_loaded_asset(rtg, only_if_is_dirty=False)

# ---------- 애니 옮기기 ----------
anims = [unreal.EditorAssetLibrary.find_asset_data(p) for p in eal.list_assets(WIZARD_ANIMS, recursive=False)
         if isinstance(unreal.load_asset(p), unreal.AnimSequence)]
created = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    anims,
    unreal.load_asset(WIZARD_MESH),
    unreal.load_asset(CONDUCTOR_MESH),
    rtg,
    "A_Wizard_",
    "A_Conductor_",
    "",
    "",
    False,
    True)

# 만든 애니는 마법사 폴더 옆에 생김 → 지휘자 폴더로 옮김
if not eal.does_directory_exist(ANIM_DIR):
    eal.make_directory(ANIM_DIR)
moved = []
for data in created:
    src = str(data.package_name)
    dst = f"{ANIM_DIR}/{src.rsplit('/', 1)[1]}"
    if src != dst:
        if eal.does_asset_exist(dst):
            eal.delete_asset(dst)
        eal.rename_asset(src, dst)
    moved.append(dst.rsplit("/", 1)[1])
print("옮긴 애니", len(moved), moved)
print("RETARGET_CONDUCTOR_COMPLETE")
