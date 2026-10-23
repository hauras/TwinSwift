# 탄막 지휘자 메시 임포트 (단위를 cm로 고친 v2 FBX, Tools/Art/fix_conductor_units.py로 만든 것)
# 처음 임포트한 것은 뿌리 뼈 크기 100이라 리타깃 애니에서 몸이 1/100로 줄었음 → 메시·스켈레톤·리타깃 결과를 지우고 다시
# 재질·텍스처(M_Conductor, T_Conductor_*)는 그대로 두고 다시 연결
# 다 하면 retarget_conductor.py를 다시 실행
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary

FBX = "C:/Users/HJB/Downloads/탄막지휘자/Conductor_Rig_v2/SK_Conductor_Rig.fbx"
BASE = "/Game/Assets/Characters/Boss/Conductor"
MESH_DIR = f"{BASE}/Mesh"
MATERIAL = f"{BASE}/Materials/M_Conductor"

# 지울 것: 옛 메시·스켈레톤에 묶인 에셋 (리타깃 애니, 지휘자 IK 리그, 리타기터)
OLD = [f"{BASE}/Retarget/RTG_Wizard_To_Conductor", f"{BASE}/Retarget/IK_Conductor",
       f"{MESH_DIR}/SK_Conductor", f"{MESH_DIR}/SK_Conductor_Skeleton", f"{MESH_DIR}/SK_Conductor_PhysicsAsset"]
if eal.does_directory_exist(f"{BASE}/Animations"):
    OLD = eal.list_assets(f"{BASE}/Animations", recursive=False) + OLD
for path in OLD:
    path = path.split(".")[0]
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
        print("지움", path)

options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
options.set_editor_property("create_physics_asset", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
options.skeletal_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)

task = unreal.AssetImportTask()
task.set_editor_property("filename", FBX)
task.set_editor_property("destination_path", MESH_DIR)
task.set_editor_property("destination_name", "SK_Conductor")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

mesh = unreal.load_asset(f"{MESH_DIR}/SK_Conductor")
material = unreal.load_asset(MATERIAL)
mats = list(mesh.get_editor_property("materials"))
for m in mats:
    m.set_editor_property("material_interface", material)
mesh.set_editor_property("materials", mats)
eal.save_loaded_asset(mesh, only_if_is_dirty=False)

b = mesh.get_bounds()
print("메시", mesh.get_path_name(), "스켈레톤", mesh.get_editor_property("skeleton").get_path_name())
print("크기", b.box_extent, "재질 슬롯", len(mats))
print("IMPORT_CONDUCTOR_COMPLETE")
