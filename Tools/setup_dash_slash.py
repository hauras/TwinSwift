# 단계 7-1 돌진 베기 에셋: 몽타주 + 능력 블루프린트 + 전사 시작 능력에 추가
#   AM_Warrior_DashSlash: A_Knight_SlideAttack (제자리 버전, 0.2~1.2초 미끄러짐, 1.2~1.7초 베기)
#   GA_Warrior_DashSlash (부모 USGA_DashSlash): 600cm / 0.6초 밀기, 1.8배, 재생 1.6배속, 0.15초부터
# 조건: USGA_DashSlash가 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

ANIM = "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_SlideAttack"
MONTAGE = "/Game/Assets/Characters/Warrior/Montages/AM_Warrior_DashSlash"
ABILITY = "/Game/Abilities/Warrior/GA_Warrior_DashSlash"
SETTINGS = dict(
    dash_distance=600.0,
    dash_duration=0.6,
    damage_multiplier=1.8,
    montage_play_rate=1.6,
    montage_start_time=0.15,
)


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 몽타주 ----------
if eal.does_asset_exist(MONTAGE):
    montage = load(MONTAGE)
else:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", load(ANIM))
    folder, name = MONTAGE.rsplit("/", 1)
    montage = tools.create_asset(name, folder, unreal.AnimMontage, factory)
for prop, value in (("blend_in", 0.08), ("blend_out", 0.2)):
    blend = montage.get_editor_property(prop)
    blend.set_editor_property("blend_time", value)
    montage.set_editor_property(prop, blend)
eal.save_loaded_asset(montage, only_if_is_dirty=False)

# ---------- 능력 블루프린트 ----------
folder, name = ABILITY.rsplit("/", 1)
if eal.does_asset_exist(ABILITY):
    bp = load(ABILITY)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.SGA_DashSlash)
    bp = tools.create_asset(name, folder, unreal.Blueprint, factory)
cdo = unreal.get_default_object(bp.generated_class())
for key, value in SETTINGS.items():
    cdo.set_editor_property(key, value)
cdo.set_editor_property("dash_montage", montage)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)
print(name, {k: cdo.get_editor_property(k) for k in SETTINGS}, "몽타주", cdo.get_editor_property("dash_montage").get_name())

# ---------- 전사 시작 능력에 추가 (이미 있으면 그대로) ----------
player_bp = load("/Game/Blueprints/Characters/BP_Player")
player = unreal.get_default_object(player_bp.generated_class())
ability = bp.generated_class()
config = player.get_editor_property("warrior_config")
text = config.export_text()
current = [s for s in text.split("StartupAbilities=(")[1].split("),DefaultAttributesEffect")[0].split(",") if s] if "StartupAbilities=(" in text else []
if not any(ability.get_path_name() in c for c in current):
    current.append(f'"{ability.get_path_name()}"')
effect = text.split("DefaultAttributesEffect=")[1].rstrip(")") if "DefaultAttributesEffect=" in text else "None"
new = unreal.TwinSlotConfig()
# 구조체 칸이 EditDefaultsOnly라 텍스트로 한 번에 넣음
new.import_text(f"(StartupAbilities=({','.join(current)}),DefaultAttributesEffect={effect})")
player.set_editor_property("warrior_config", new)
print("warrior_config", player.get_editor_property("warrior_config").export_text())
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
