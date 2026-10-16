# 전사 콤보: 타마다 몽타주 하나 + GA_Warrior_Attack의 ComboSteps
# 애니는 같은 대검 준비 자세로 시작·끝남(자세 차이 0cm) → 코드가 이어 붙여도 튀지 않음
# 판정·궤적·베기 이펙트 구간은 이 스크립트 다음에 setup_slash_fx.py로 넣음 (칼끝 궤적으로 계산)
# 조건: FSComboStep이 들어간 C++이 빌드돼 있어야 함
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
anim_lib = unreal.AnimationLibrary
ev = unreal.AnimPoseEvaluationOptions()

ANIM_DIR = "/Game/Assets/External/Characters/Knight_Free/Animations"
MONTAGE_DIR = "/Game/Assets/Characters/Warrior/Montages"

# (몽타주 이름, 애니, 배율)
# 2타 콤보: 대각선 내려치기 → 가로 베기(마무리)
# AM_Warrior_Combo2(Slash_02)는 뺌: 1.8초 중 1.4초가 머리 위 칼 돌리기라 공격으로 안 보이고 리듬이 처짐
#   좋은 애니를 구하면 여기 한 줄 추가 + setup_slash_fx.py의 STRIKES에 추가
STEPS = [
    ("AM_Warrior_Combo1", "A_Knight_Slash_03", 1.0),
    ("AM_Warrior_Combo3", "A_Knight_Slash_01", 1.6),
]

# 블렌드: 같은 자세로 이어지니 짧게
BLEND_IN = 0.1
BLEND_OUT = 0.2


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


montages = []
for montage_name, anim_name, multiplier in STEPS:
    path = f"{MONTAGE_DIR}/{montage_name}"
    anim = load(f"{ANIM_DIR}/{anim_name}")
    if eal.does_asset_exist(path):
        montage = load(path)
    else:
        factory = unreal.AnimMontageFactory()
        factory.set_editor_property("source_animation", anim)
        montage = tools.create_asset(montage_name, MONTAGE_DIR, unreal.AnimMontage, factory)
        print("만듦:", path)

    for prop, value in (("blend_in", BLEND_IN), ("blend_out", BLEND_OUT)):
        blend = montage.get_editor_property(prop)
        blend.set_editor_property("blend_time", value)
        montage.set_editor_property(prop, blend)

    eal.save_loaded_asset(montage, only_if_is_dirty=False)

    print(f"{montage_name} ({anim_name}, {montage.get_play_length():.2f}초, 배율 {multiplier})")
    montages.append((montage, multiplier))

# ---------- GA_Warrior_Attack: ComboSteps ----------
ga_bp = load("/Game/Abilities/Warrior/GA_Warrior_Attack")
ga_cdo = unreal.get_default_object(ga_bp.generated_class())
steps = []
for montage, multiplier in montages:
    step = unreal.SComboStep()
    # 구조체 칸이 EditDefaultsOnly라 텍스트로 한 번에 넣음
    step.import_text(f'(Montage="{montage.get_path_name()}",DamageMultiplier={multiplier})')
    steps.append(step)
ga_cdo.set_editor_property("combo_steps", steps)
ga_cdo.set_editor_property("combo_reset_time", 1.0)
unreal.BlueprintEditorLibrary.compile_blueprint(ga_bp)
eal.save_loaded_asset(ga_bp, only_if_is_dirty=False)

for s in ga_cdo.get_editor_property("combo_steps"):
    print("ComboStep:", s.export_text())

# 1타짜리 예전 몽타주 정리 (Combo1과 같은 애니)
OLD = f"{MONTAGE_DIR}/AM_Warrior_Attack"
if eal.does_asset_exist(OLD):
    print("예전 몽타주 삭제:", eal.delete_asset(OLD))
