# 마법사(Air_Wizard) 모델 + 믹사모 Pro Magic Pack 애니 임포트
# - 모델: Wizard_Free(기사와 같은 제작자). 뼈 이름·구조가 기사와 같고 뿌리 뼈 이름만 다름(Air_Wizard_metarig) → 자기 스켈레톤으로
# - 재질: 팔레트 텍스처가 기사와 같은 파일 → 기사 재질 M_TS_Knight를 그대로 씀
# - 애니: 믹사모에 Air_Wizard를 올려 받은 팩이라 뼈가 마법사와 같음 → 리타깃 없이 마법사 스켈레톤에 바로
# 다시 실행해도 이미 있는 에셋은 건너뜀
import os
import re

import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

MESH_FBX = "C:/Users/HJB/Desktop/Wizard_Free/FBX/Characters/Air_Wizard.fbx"
MESH_DEST = "/Game/Asset/Wizard_Free/FBX/Characters"
MESH = f"{MESH_DEST}/Air_Wizard"
ANIM_DIR = "C:/Users/HJB/Downloads"
ANIM_DEST = "/Game/Asset/Wizard_Free/Animations"
MATERIAL = "/Game/Asset/Knight_Free/M_TS_Knight"

# Pro Magic Pack(56개) 중 쓰는 것만 (앉기·막기·옆/뒤 걷기·점프·돌기 등은 2026-10-16 정리하며 뺌)
PACK_FILES = [
    "standing idle", "Standing Walk Forward", "Standing Run Forward", "Standing Sprint Forward",
    "Standing 1H Magic Attack 01", "Standing 1H Magic Attack 02", "Standing 1H Magic Attack 03",
    "standing 1H cast spell 01", "Standing 2H Cast Spell 01",
    "Standing 2H Magic Area Attack 01", "Standing 2H Magic Area Attack 02",
    "Standing 2H Magic Attack 01", "Standing 2H Magic Attack 02", "Standing 2H Magic Attack 03",
    "Standing 2H Magic Attack 04", "Standing 2H Magic Attack 05",
    "Standing React Small From Front", "Standing React Large From Front",
    "Standing React Death Backward", "Standing React Death Forward",
]


# 믹사모에서 In Place를 켜고 따로 받은 파일 (팩의 걷기·달리기는 앞으로 나감) → 이 파일로 임포트
IN_PLACE_FILES = {
    "Standing Walk Forward": "Standing Walk Forward (1)",
    "Standing Run Forward": "Standing Run Forward (1)",
    "Standing Sprint Forward": "Standing Sprint Forward (1)",
}


def asset_name(file_name):
    # "Standing 1H Magic Attack 01" → A_Wizard_1HMagicAttack_01, "standing idle" → A_Wizard_Idle
    words = re.sub(r"^standing\s+", "", file_name, flags=re.I).split()
    number = words.pop() if words and words[-1].isdigit() and len(words[-1]) == 2 else None
    name = "".join(w[:1].upper() + w[1:] for w in words)
    return f"A_Wizard_{name}" + (f"_{number}" if number else "")


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 모델 ----------
if not eal.does_asset_exist(MESH):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("create_physics_asset", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.skeletal_mesh_import_data.set_editor_property("convert_scene", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", MESH_FBX)
    task.set_editor_property("destination_path", MESH_DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tools.import_asset_tasks([task])

mesh = load(MESH)
material = load(MATERIAL)
# 구조체 배열은 꺼낸 값이 복사본이라 칸을 고쳐도 반영 안 됨 → 텍스트로 새 구조체를 만들어 통째로 넣음
materials = []
for slot in mesh.get_editor_property("materials"):
    text = re.sub(r'MaterialInterface=[^,]+', f'MaterialInterface="{material.get_path_name()}"', slot.export_text())
    new_slot = unreal.SkeletalMaterial()
    new_slot.import_text(text)
    materials.append(new_slot)
mesh.set_editor_property("materials", materials)
eal.save_loaded_asset(mesh, only_if_is_dirty=False)
skeleton = mesh.skeleton
print("모델", mesh.get_path_name(), "스켈레톤", skeleton.get_path_name(), "재질 칸", len(materials),
      "크기", mesh.get_bounds().box_extent)

# ---------- 애니 ----------
tasks = []
for file_name in PACK_FILES:
    name = asset_name(file_name)
    if eal.does_asset_exist(f"{ANIM_DEST}/{name}"):
        continue
    path = f"{ANIM_DIR}/{IN_PLACE_FILES.get(file_name, file_name)}.fbx"
    if not os.path.exists(path):
        print("파일 없음", path)
        continue
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("skeleton", skeleton)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    # 기사 애니와 같은 설정
    anim = ui.get_editor_property("anim_sequence_import_data")
    anim.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    anim.set_editor_property("import_bone_tracks", True)
    anim.set_editor_property("remove_redundant_keys", True)
    anim.set_editor_property("convert_scene", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", ANIM_DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append(task)
if tasks:
    tools.import_asset_tasks(tasks)

ok = 0
for file_name in PACK_FILES:
    name = asset_name(file_name)
    a = unreal.load_asset(f"{ANIM_DEST}/{name}.{name}")
    if a and a.get_editor_property("skeleton") == skeleton:
        ok += 1
    else:
        print("실패", name)
print(f"애니 {ok}/{len(PACK_FILES)}개")
