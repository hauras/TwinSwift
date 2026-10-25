# 능력 입력 액션 + IMC_Default 키 매핑 + DA_AbilityInputConfig(입력 → 태그) + BP_PlayerController 연결
# 이름은 역할 기준. 키를 바꿀 때는 아래 KEYS만 고치고 다시 실행
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# (입력 액션, 키, 입력 태그)
KEYS = [
    ("IA_Attack", "LeftMouseButton", "InputTag.Attack"),
    ("IA_Warrior_Skill1", "One", "InputTag.Warrior.Skill1"),
    ("IA_Warrior_Skill2", "Two", "InputTag.Warrior.Skill2"),
    ("IA_Mage_Skill1", "Three", "InputTag.Mage.Skill1"),
    ("IA_Mage_Skill2", "Four", "InputTag.Mage.Skill2"),
    ("IA_Dodge", "SpaceBar", "InputTag.Dodge"),
]


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


# ---------- 입력 액션 ----------
actions = {}
for ia_name, _, _ in KEYS:
    path = f"/Game/Input/{ia_name}"
    if eal.does_asset_exist(path):
        ia = eal.load_asset(path)
    else:
        ia = tools.create_asset(ia_name, "/Game/Input", unreal.InputAction, unreal.InputAction_Factory())
        print("만듦:", path)
    ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    eal.save_loaded_asset(ia, only_if_is_dirty=False)
    actions[ia_name] = ia

# ---------- IMC_Default: 능력 입력만 다시 매핑 (이동·카메라 매핑은 그대로) ----------
imc = eal.load_asset("/Game/Input/IMC_Default")
for ia in actions.values():
    imc.unmap_all_keys_from_action(ia)
for ia_name, key_name, _ in KEYS:
    imc.map_key(actions[ia_name], key(key_name))
eal.save_loaded_asset(imc, only_if_is_dirty=False)

# ---------- DA_AbilityInputConfig (C++ USInputConfig, 태그가 등록돼 있어야 함) ----------
def tag(name):
    t = unreal.GameplayTag()
    t.import_text(f'(TagName="{name}")')
    return t

missing = [n for _, _, n in KEYS if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag(n))]
if missing:
    print("태그가 아직 등록 안 됨 (C++ 빌드 후 다시 실행):", missing)
    raise SystemExit

cfg_path = "/Game/Input/DA_AbilityInputConfig"
if eal.does_asset_exist(cfg_path):
    cfg = eal.load_asset(cfg_path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.SInputConfig)
    cfg = tools.create_asset("DA_AbilityInputConfig", "/Game/Input", unreal.SInputConfig, factory)
    print("만듦:", cfg_path)

entries = []
for ia_name, _, tag_name in KEYS:
    # 구조체 칸이 EditDefaultsOnly라 칸 단위로는 못 넣음 → 텍스트로 한 번에
    e = unreal.SInputActionConfig()
    e.import_text(f'(InputAction="{actions[ia_name].get_path_name()}",InputTag=(TagName="{tag_name}"))')
    entries.append(e)
cfg.set_editor_property("ability_input_actions", entries)
eal.save_loaded_asset(cfg, only_if_is_dirty=False)

# ---------- BP_PlayerController에 연결 ----------
bp = unreal.load_asset("/Game/Blueprints/Core/BP_PlayerController.BP_PlayerController")
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("ability_input_config", cfg)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)

# ---------- 확인 ----------
for e in cfg.get_editor_property("ability_input_actions"):
    t = e.get_editor_property("input_tag")
    print(e.get_editor_property("input_action").get_name(), "->", t.export_text())
