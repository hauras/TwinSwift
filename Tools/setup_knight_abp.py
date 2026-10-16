# 기사 애님 BP 만들기 + BP_Player를 애님 BP 방식으로 바꾸기 + 블렌드스페이스 축 정리
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

skeleton = unreal.load_asset("/Game/Assets/External/Characters/Knight_Free/FBX/Characters/Knight_01_Skeleton")

# ---------- ABP_Knight ----------
abp_path = "/Game/Assets/Characters/Warrior/ABP_Knight"
if eal.does_asset_exist(abp_path):
    abp = eal.load_asset(abp_path)
    print("이미 있음:", abp_path)
else:
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("target_skeleton", skeleton)
    factory.set_editor_property("parent_class", unreal.CharacterAnimInstance)
    abp = asset_tools.create_asset("ABP_Knight", "/Game/Assets/Characters/Warrior", unreal.AnimBlueprint, factory)
    print("만듦:", abp_path)
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
eal.save_loaded_asset(abp, only_if_is_dirty=False)

# ---------- BP_Player: Single Node → 애님 BP ----------
bp_player = eal.load_asset("/Game/Blueprints/BP_Player")
cdo = unreal.get_default_object(bp_player.generated_class())
mesh = cdo.get_editor_property("mesh")
mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
mesh.set_editor_property("anim_class", abp.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(bp_player)
eal.save_loaded_asset(bp_player, only_if_is_dirty=False)
print("BP_Player: 애님 BP =", mesh.get_editor_property("anim_class").get_name())

# ---------- 블렌드스페이스: 축 0~500, 중복 Run(400) 삭제 ----------
bs = unreal.load_asset("/Game/Assets/Characters/Warrior/BS_Knight_Locomotion")
params = list(bs.get_editor_property("blend_parameters"))
params[0].set_editor_property("max", 500.0)
params[0].set_editor_property("grid_num", 5)
bs.set_editor_property("blend_parameters", params)

samples = [s for s in bs.get_editor_property("sample_data")
           if not (s.get_editor_property("animation").get_name() == "A_Knight_Run"
                   and abs(s.get_editor_property("sample_value").x - 400.0) < 1.0)]
bs.set_editor_property("sample_data", samples)
eal.save_loaded_asset(bs, only_if_is_dirty=False)

bs = unreal.load_asset("/Game/Assets/Characters/Warrior/BS_Knight_Locomotion")
p = bs.get_editor_property("blend_parameters")[0]
print("블렌드스페이스 축:", p.get_editor_property("min"), "~", p.get_editor_property("max"))
for s in bs.get_editor_property("sample_data"):
    print("  ", s.get_editor_property("animation").get_name(), s.get_editor_property("sample_value").x)
