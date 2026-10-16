# 대검 애니 FBX를 기사 스켈레톤(Knight_01_Skeleton)에 바로 임포트
# FBX 뼈 이름이 기사와 같아서(Knight_metarig_01, spine, hand_R ...) 리타깃 없이 그대로 씀
# 새 애니를 받으면 아래 ANIMS에 (파일 이름, 에셋 이름)만 추가하고 다시 실행 (이미 있는 에셋은 건너뜀)
import unreal

SOURCE_DIR = "C:/Users/HJB/Downloads"
DEST = "/Game/Assets/External/Characters/Knight_Free/Animations"
SKELETON = unreal.load_asset("/Game/Assets/External/Characters/Knight_Free/FBX/Characters/Knight_01_Skeleton")

ANIMS = [
    ("Great Sword Attack.fbx", "A_Knight_Attack"),
    ("Great Sword Slash.fbx", "A_Knight_Slash"),
    ("Great Sword Slash (1).fbx", "A_Knight_Slash_01"),
    ("Great Sword Slash (2).fbx", "A_Knight_Slash_02"),
    ("Great Sword Slash (3).fbx", "A_Knight_Slash_03"),
    ("Stand To Roll.fbx", "A_Knight_Roll"),
    ("Great Sword Slide Attack.fbx", "A_Knight_SlideAttack"),
    ("Great Sword Jump Attack.fbx", "A_Knight_JumpAttack"),
    ("Great Sword High Spin Attack.fbx", "A_Knight_SpinAttack"),
    ("Great Sword Jump.fbx", "A_Knight_Jump"),
    ("Great Sword Casting.fbx", "A_Knight_Casting"),
]

tasks = []
for file_name, asset_name in ANIMS:
    if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{asset_name}"):
        continue
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("skeleton", SKELETON)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)

    # 기존 A_Knight_Idle과 같은 설정
    anim = ui.get_editor_property("anim_sequence_import_data")
    anim.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    anim.set_editor_property("import_bone_tracks", True)
    anim.set_editor_property("remove_redundant_keys", True)
    anim.set_editor_property("convert_scene", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", f"{SOURCE_DIR}/{file_name}")
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for _, asset_name in ANIMS:
    a = unreal.load_asset(f"{DEST}/{asset_name}.{asset_name}")
    if a:
        print("OK", asset_name, "skeleton:", a.get_editor_property("skeleton").get_name(), "len:", round(a.get_play_length(), 2))
    else:
        print("실패", asset_name)
