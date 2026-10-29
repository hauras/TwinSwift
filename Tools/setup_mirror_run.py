# 거울 보스를 런에 넣음 (시험용: StartIndex = 2면 거울부터. 시험이 끝나면 START_INDEX = 0으로 다시 실행)
# 확인용 배치(Boss_Mirror_Check)는 지움 (런이 생성함)
# 다시 실행해도 됨
import unreal

START_INDEX = 2

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

if not hasattr(unreal, "SActionReplayComponent"):
    raise RuntimeError("재생 컴포넌트가 아직 빌드 안 됨")

gm_bp = unreal.load_asset("/Game/Blueprints/Core/BP_GameMode")
gm = unreal.get_default_object(gm_bp.generated_class())
order = [unreal.load_asset(f"/Game/Blueprints/Boss/{name}").generated_class()
         for name in ["BP_Boss_Golem", "BP_Boss_Conductor", "BP_Boss_Mirror"]]
print("전:", [c.get_name() for c in gm.get_editor_property("boss_character_classes")], "/ 시작", gm.get_editor_property("start_index"))
gm.set_editor_property("boss_character_classes", order)
gm.set_editor_property("start_index", START_INDEX)
unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
unreal.EditorAssetLibrary.save_loaded_asset(gm_bp)
print("후:", [c.get_name() for c in gm.get_editor_property("boss_character_classes")], "/ 시작", gm.get_editor_property("start_index"))

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed = 0
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == "Boss_Mirror_Check":
        actors.destroy_actor(actor)
        removed += 1
if removed:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("확인용 배치 지움:", removed)

boss = unreal.get_default_object(order[2])
print("거울 보스 재생 컴포넌트:", boss.get_components_by_class(unreal.SActionReplayComponent))
print("MIRROR_RUN_COMPLETE")
