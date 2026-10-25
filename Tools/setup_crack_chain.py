# 단계 8-3 균열 연쇄 폭발: 연쇄 확인용 더미 배치 + 확인용 연쇄 간격 (L_Dev, GA_Mage_Thunder)
#   더미 일렬 270 간격: 이웃은 폭발 반경(300) 안, 낙뢰(250)는 첫 더미만 → 1 → 2 → 3 → 4단계가 차례로
#   표식은 콘솔 DebugMarkAll Crack 으로 한 번에 (내려찍기 반경 300으로는 4개를 다 못 찍음)
#   폭발 이펙트는 VFX/make_crack_burst.py가 지정 (NS_CrackBurst)
# 조건: SGA_ThunderStrike에 연쇄 속성이 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

THUNDER = "/Game/Abilities/Mage/GA_Mage_Thunder"
DUMMY_BP = "/Game/Blueprints/Debug/BP_Dummy"

# 눈으로 단계를 구분하기 쉬운 간격 (C++ 기본 0.12). 느낌을 본 뒤 줄임
CHAIN_STEP_DELAY = 0.25

DUMMIES = {
    "Dummy": unreal.Vector(400.0, 0.0, 100.0),
    "Dummy_Chain1": unreal.Vector(670.0, 0.0, 100.0),
    "Dummy_Chain2": unreal.Vector(940.0, 0.0, 100.0),
    "Dummy_Chain3": unreal.Vector(1210.0, 0.0, 100.0),
}

# 1. 연쇄 간격
bp = unreal.load_asset(THUNDER)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("chain_step_delay", CHAIN_STEP_DELAY)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
cdo = unreal.get_default_object(bp.generated_class())
effect = cdo.get_editor_property("burst_effect")
print(THUNDER, "delay", cdo.get_editor_property("chain_step_delay"),
      "effect", effect.get_name() if effect else None)

# 2. 더미 (없으면 만들고, 있으면 위치만 맞춤)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
dummy_class = unreal.load_asset(DUMMY_BP).generated_class()
for label, location in DUMMIES.items():
    actor = existing.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(dummy_class, location, unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
        actor.set_actor_label(label)
    actor.set_actor_location(location, False, False)
    actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0), False)
    print(label, actor.get_actor_location())

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
