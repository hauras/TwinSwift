# 거울 보스 1단계: BP_Player에 기록 컴포넌트(SActionRecorderComponent) 붙이기
# 위치(0.1초마다)·행동(능력 발동·교체)을 원형 버퍼에 기록 → 거울 보스가 3초 전 기록을 읽어 재생
# 확인: 콘솔 ts.Mirror.ShowGhost 1 → 3초 전 내 위치에 보라 구 + 행동 로그
# 다시 실행해도 됨 (이미 있으면 그대로)
import unreal


def run():
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
        raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

    bp = unreal.load_asset("/Game/Blueprints/Characters/BP_Player")
    cls = unreal.load_class(None, "/Script/TwinSwift.SActionRecorderComponent")
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    names = [str(lib.get_variable_name(lib.get_data(h))) for h in handles]
    if "ActionRecorder" in names:
        print("이미 있음:", names)
    else:
        params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=cls, blueprint_context=bp)
        handle, fail = subsystem.add_new_subobject(params)
        if not fail.is_empty():
            raise RuntimeError(f"추가 실패: {fail}")
        subsystem.rename_subobject(handle, unreal.Text("ActionRecorder"))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    print("ACTION_RECORDER_COMPLETE")


run()
