# 단계 5-1 에셋: 교체 키(Tab) + 임시 마법사 몸
# 1. IA_Swap 만들고 IMC_Default에 Tab, BP_PlayerController의 SwapAction에 연결
# 2. 임시 마법사 재질 M_TS_MageTemp: 기사 재질(M_TS_Knight) 복사 + 기본 색에 파란색을 곱함
# 3. BP_Player의 MageMesh: 기사 메시 + ABP_Knight + 임시 재질 (마법사 에셋을 찾으면 메시·애니만 바꿈)
# 다시 실행해도 같은 결과 (이미 있으면 덮어씀)
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_lib = unreal.MaterialEditingLibrary

SWAP_KEY = "Tab"
# 임시 마법사 색: 기사 텍스처 색에 곱함 (1보다 크면 밝아짐)
MAGE_TINT = unreal.LinearColor(0.35, 0.55, 1.4, 1.0)


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 1. 교체 입력 ----------
ia_path = "/Game/Input/IA_Swap"
if eal.does_asset_exist(ia_path):
    ia = load(ia_path)
else:
    ia = tools.create_asset("IA_Swap", "/Game/Input", unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
eal.save_loaded_asset(ia, only_if_is_dirty=False)

imc = load("/Game/Input/IMC_Default")
imc.unmap_all_keys_from_action(ia)
key = unreal.Key()
key.set_editor_property("key_name", SWAP_KEY)
imc.map_key(ia, key)
eal.save_loaded_asset(imc, only_if_is_dirty=False)

pc_bp = load("/Game/Blueprints/BP_PlayerController")
pc_cdo = unreal.get_default_object(pc_bp.generated_class())
pc_cdo.set_editor_property("swap_action", ia)
unreal.BlueprintEditorLibrary.compile_blueprint(pc_bp)
eal.save_loaded_asset(pc_bp, only_if_is_dirty=False)
print("교체 키:", SWAP_KEY, "→", ia.get_path_name())

# ---------- 2. 임시 마법사 재질 ----------
knight_mat_path = "/Game/Assets/Characters/Warrior/M_TS_Knight"
mage_mat_path = "/Game/Assets/Characters/Mage/M_TS_MageTemp"
if eal.does_asset_exist(mage_mat_path):
    eal.delete_asset(mage_mat_path)
mage_mat = eal.duplicate_asset(knight_mat_path, mage_mat_path)

# 기본 색으로 들어가던 노드 뒤에 "× 파란색"을 끼워 넣음
base_node = mat_lib.get_material_property_input_node(mage_mat, unreal.MaterialProperty.MP_BASE_COLOR)
tint = mat_lib.create_material_expression(mage_mat, unreal.MaterialExpressionConstant3Vector, -300, 300)
tint.set_editor_property("constant", MAGE_TINT)
multiply = mat_lib.create_material_expression(mage_mat, unreal.MaterialExpressionMultiply, -150, 200)
if base_node:
    mat_lib.connect_material_expressions(base_node, "", multiply, "A")
mat_lib.connect_material_expressions(tint, "", multiply, "B")
mat_lib.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)
mat_lib.recompile_material(mage_mat)
eal.save_loaded_asset(mage_mat, only_if_is_dirty=False)
print("임시 마법사 재질:", mage_mat_path, "(원래 기본 색 노드:", base_node.get_name() if base_node else None, ")")

# ---------- 3. BP_Player의 마법사 몸 ----------
bp = load("/Game/Blueprints/BP_Player")
cdo = unreal.get_default_object(bp.generated_class())
warrior_mesh = cdo.get_editor_property("mesh")
mage_mesh = cdo.get_editor_property("mage_mesh")
mage_mesh.set_skinned_asset_and_update(warrior_mesh.get_skinned_asset())
mage_mesh.set_editor_property("anim_class", warrior_mesh.get_editor_property("anim_class"))
mage_mesh.set_material(0, mage_mat)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)
print("MageMesh:", mage_mesh.get_skinned_asset().get_name(), mage_mesh.get_editor_property("anim_class").get_name(), mage_mesh.get_material(0).get_name())
