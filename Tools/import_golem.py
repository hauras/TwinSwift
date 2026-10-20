# 골렘 보스: 메시 1번 + 애니메이션은 같은 스켈레톤에 바로 임포트 (믹사모에서 같은 리깅으로 받아 뼈가 같음 → 리타깃 불필요)
# AI로 만든 모델이 미터 단위라 그대로 넣으면 키가 1cm → 메시·애니 모두 ×100
# 새 애니를 받으면 ANIMS에 (파일 이름, 에셋 이름)만 추가하고 다시 실행 (이미 있는 에셋은 건너뜀)
import unreal

SOURCE_DIR = "C:/Users/HJB/Downloads"
TEX_DIR = "C:/Users/HJB/Downloads/0d6e36f8-33b8-4b25-83f3-5d477be0b2ca"
ROOT = "/Game/Assets/Characters/Boss/Golem"
MESH_DIR = f"{ROOT}/Mesh"
ANIM_DIR = f"{ROOT}/Animations"
TEX_DEST = f"{ROOT}/Textures"
MAT_DIR = f"{ROOT}/Materials"
SCALE = 100.0

MESH_FILE = "Mutant Idle.fbx"
ANIMS = [
    ("Mutant Idle.fbx", "A_Golem_Idle"),
    # 걷기·달리기는 믹사모 In Place로 다시 받은 파일 (처음 받은 것은 골반이 한 바퀴에 3m씩 앞으로 가서 튕김)
    ("Mutant Walking (1).fbx", "A_Golem_Walk"),
    ("Mutant Run (1).fbx", "A_Golem_Run"),
    ("Mutant Punch.fbx", "A_Golem_Punch"),
    ("Mutant Swiping.fbx", "A_Golem_Swipe"),
    ("Jump Attack.fbx", "A_Golem_JumpAttack"),
    ("Mutant Dying.fbx", "A_Golem_Death"),
]
TEXTURES = [
    ("texture_diffuse.png", "T_Golem_D"),
    ("texture_normal.png", "T_Golem_N"),
    ("texture_roughness.png", "T_Golem_R"),
    ("texture_metallic.png", "T_Golem_M"),
    ("texture_emissive.png", "T_Golem_E"),
]

# 이미 있어도 덮어쓸 애니 (파일을 새로 받았을 때 여기에 에셋 이름을 넣고 실행 → 끝나면 비워 둠)
# 지우고 새로 만들지 않고 덮어써서 블렌드스페이스·몽타주 연결이 유지됨
REIMPORT = set()

eal = unreal.EditorAssetLibrary

if unreal.EditorLevelLibrary.get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")


def make_task(file_path, dest, name, options):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", file_path)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    if options:
        task.set_editor_property("options", options)
    return task


# 1) 메시 (1번만)
if not eal.does_asset_exist(f"{MESH_DIR}/SK_Golem"):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("create_physics_asset", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.skeletal_mesh_import_data.set_editor_property("import_uniform_scale", SCALE)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(
        [make_task(f"{SOURCE_DIR}/{MESH_FILE}", MESH_DIR, "SK_Golem", ui)])

mesh = unreal.load_asset(f"{MESH_DIR}/SK_Golem")
skeleton = mesh.skeleton

# 임포트 옵션(create_physics_asset)이 무시돼서 따로 만듦
if not mesh.get_editor_property("physics_asset"):
    pa = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).create_physics_asset(mesh)
    eal.save_loaded_asset(pa)
    eal.rename_asset(pa.get_path_name().split(".")[0], f"{MESH_DIR}/PA_Golem")
    eal.save_loaded_asset(mesh)

# 2) 애니메이션: 메시 없이 SK_Golem의 스켈레톤에
tasks = []
for file_name, asset_name in ANIMS:
    if eal.does_asset_exist(f"{ANIM_DIR}/{asset_name}") and asset_name not in REIMPORT:
        continue
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("skeleton", skeleton)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    anim = ui.get_editor_property("anim_sequence_import_data")
    anim.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    anim.set_editor_property("import_bone_tracks", True)
    anim.set_editor_property("remove_redundant_keys", True)
    anim.set_editor_property("import_uniform_scale", SCALE)
    tasks.append(make_task(f"{SOURCE_DIR}/{file_name}", ANIM_DIR, asset_name, ui))
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

# 3) 텍스처
tasks = [make_task(f"{TEX_DIR}/{f}", TEX_DEST, n, None)
         for f, n in TEXTURES if not eal.does_asset_exist(f"{TEX_DEST}/{n}")]
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for _, n in TEXTURES:
    t = unreal.load_asset(f"{TEX_DEST}/{n}")
    if n.endswith("_N"):
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        t.set_editor_property("srgb", False)
    elif n.endswith("_R") or n.endswith("_M"):
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        t.set_editor_property("srgb", False)
    eal.save_loaded_asset(t)

# 4) 재질
MEL = unreal.MaterialEditingLibrary
if not eal.does_asset_exist(f"{MAT_DIR}/M_Golem"):
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Golem", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    pins = [
        ("T_Golem_D", unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, 0),
        ("T_Golem_N", unreal.MaterialProperty.MP_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 1),
        ("T_Golem_R", unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 2),
        ("T_Golem_M", unreal.MaterialProperty.MP_METALLIC, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 3),
        ("T_Golem_E", unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, 4),
    ]
    for tex_name, prop, sampler, row in pins:
        node = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, row * 260)
        node.set_editor_property("texture", unreal.load_asset(f"{TEX_DEST}/{tex_name}"))
        node.set_editor_property("sampler_type", sampler)
        MEL.connect_material_property(node, "RGB" if prop != unreal.MaterialProperty.MP_ROUGHNESS and prop != unreal.MaterialProperty.MP_METALLIC else "R", prop)
    MEL.recompile_material(mat)
    eal.save_loaded_asset(mat)

mat = unreal.load_asset(f"{MAT_DIR}/M_Golem")
mats = mesh.get_editor_property("materials")
for m in mats:
    m.set_editor_property("material_interface", mat)
mesh.set_editor_property("materials", mats)
eal.save_loaded_asset(mesh)

# 확인
b = mesh.get_bounds()
print("SK_Golem 크기(반): ", round(b.box_extent.x, 1), round(b.box_extent.y, 1), round(b.box_extent.z, 1))
print("스켈레톤:", skeleton.get_path_name())
for _, n in ANIMS:
    a = unreal.load_asset(f"{ANIM_DIR}/{n}")
    print("OK" if a else "실패", n, a.get_skeleton().get_name() if a else "", round(a.get_play_length(), 2) if a else "")
