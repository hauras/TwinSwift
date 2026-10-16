# WBP_TwinHUD의 스킬 아이콘 칸(SkillMaterials)에 아이콘 머티리얼 8개 지정
# 예전에는 코드가 경로 문자열로 불러오고, 패키징에서 빠지지 않게 숨긴 이미지 묶음(IconPreload)을 뒀음 → 둘 다 없앰
# 조건: SkillMaterials가 EditDefaultsOnly인 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

NAMES = ["WarriorAttack", "DashSlash", "GroundSlam", "Roll", "MagicBolt", "FrostBolt", "Thunder", "Blink"]

bp = unreal.load_asset("/Game/UI/WBP_TwinHUD")
cdo = unreal.get_default_object(bp.generated_class())
icons = {unreal.Name(n): unreal.load_asset(f"/Game/UI/Icons/M_UI_{n}") for n in NAMES}
cdo.set_editor_property("skill_materials", icons)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
print({str(k): v.get_name() for k, v in cdo.get_editor_property("skill_materials").items()})
