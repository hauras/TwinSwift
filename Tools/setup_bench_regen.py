# 대기 중 회복 GE를 만들고 BP_Player의 BenchRegenEffect에 연결
# GE_BenchRegen: 계속 유지(Infinite), PERIOD초마다 체력 + 최대 체력 × RATE
#   (수치는 대상 캐릭터의 최대 체력을 그때그때 읽음 → 전사 120이면 1.2, 마법사 90이면 0.9씩)
# 조건: BenchRegenEffect가 들어간 C++이 빌드돼 있어야 BP에 연결됨 (없으면 GE만 만들고 안내)
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

PATH = "/Game/Abilities/Common/GE_BenchRegen"
PERIOD = 0.5
# 한 번에 최대 체력의 몇 배 (0.01 × 2번/초 = 초당 2%)
RATE = 0.01

folder, name = PATH.rsplit("/", 1)
if eal.does_asset_exist(PATH):
    bp = unreal.load_asset(PATH + "." + name)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.GameplayEffect)
    bp = tools.create_asset(name, folder, unreal.Blueprint, factory)

cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("duration_policy", unreal.GameplayEffectDurationType.INFINITE)

period = unreal.ScalableFloat()
period.import_text(f"(Value={PERIOD})")
cdo.set_editor_property("period", period)

# 체력 += 대상의 최대 체력 × RATE (붙어 있는 동안 최대 체력이 바뀌어도 새 값으로 계산)
modifier = unreal.GameplayModifierInfo()
modifier.import_text(
    '(Attribute=(AttributeName="Health",Attribute=/Script/TwinSwift.SAttributeSet:Health,AttributeOwner=/Script/TwinSwift.SAttributeSet),'
    'ModifierOp=AddBase,'
    'ModifierMagnitude=(MagnitudeCalculationType=AttributeBased,AttributeBasedMagnitude=('
    f'Coefficient=(Value={RATE}),'
    'BackingAttribute=(AttributeToCapture=(AttributeName="MaxHealth",Attribute=/Script/TwinSwift.SAttributeSet:MaxHealth,AttributeOwner=/Script/TwinSwift.SAttributeSet),'
    'AttributeSource=Target,bSnapshot=False))))')
cdo.set_editor_property("modifiers", [modifier])
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)

saved = cdo.get_editor_property("modifiers")[0].export_text()
print("GE:", PATH, "| 주기", cdo.get_editor_property("period").export_text(), "| 수정자", saved[:220])

player_bp = unreal.load_asset("/Game/Blueprints/BP_Player.BP_Player")
player_cdo = unreal.get_default_object(player_bp.generated_class())
try:
    player_cdo.set_editor_property("bench_regen_effect", bp.generated_class())
except Exception as e:
    print("BP 연결 못 함 (빌드 후 다시 실행):", e)
else:
    unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
    eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
    print("BP_Player.BenchRegenEffect =", player_cdo.get_editor_property("bench_regen_effect").get_name())
