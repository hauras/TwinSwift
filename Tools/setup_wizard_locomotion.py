# 마법사 이동 블렌드스페이스 BS_Wizard_Locomotion (기사 BS_Knight_Locomotion과 같은 축: GroundSpeed 0~500)
# 샘플 위치 = 애니가 실제로 움직이는 속도(In Place 받기 전 원본에서 잰 값) → 발이 덜 미끄러짐
#   서 있기 0, 걷기 140cm/1.13초 = 124, 달리기 218cm/0.73초 = 297, 질주 242cm/0.57초 = 424 → 최고 속도 500에 1.18배속
# Python으로 샘플을 넣으면 블렌드 계산용 데이터가 안 만들어져서(게임에서 걸어도 서 있기 자세) C++ 도우미로 다시 만듦
# 조건: UTSAnimEditLibrary가 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
D = "/Game/Asset/Wizard_Free"
PATH = f"{D}/BS_Wizard_Locomotion"

skeleton = unreal.load_asset(f"{D}/FBX/Characters/Air_Wizard_Skeleton")
if eal.does_asset_exist(PATH):
    bs = unreal.load_asset(PATH)
else:
    factory = unreal.BlendSpaceFactory1D()
    factory.set_editor_property("target_skeleton", skeleton)
    bs = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BS_Wizard_Locomotion", D, unreal.BlendSpace1D, factory)

knight = unreal.load_asset("/Game/Asset/Knight_Free/BS_Knight_Locomotion")
bs.set_editor_property("blend_parameters", knight.get_editor_property("blend_parameters"))


def sample(anim, speed, rate=1.0):
    s = unreal.BlendSample()
    s.set_editor_property("animation", unreal.load_asset(f"{D}/Animations/{anim}"))
    s.set_editor_property("sample_value", unreal.Vector(speed, 0, 0))
    s.set_editor_property("rate_scale", rate)
    return s


bs.set_editor_property("sample_data", [
    sample("A_Wizard_Idle", 0),
    sample("A_Wizard_WalkForward", 124),
    sample("A_Wizard_RunForward", 297),
    sample("A_Wizard_SprintForward", 500, 500 / 424),
])
print("블렌드 데이터 다시 만들기", unreal.TSAnimEditLibrary.rebuild_blend_space(bs))
eal.save_loaded_asset(bs, only_if_is_dirty=False)
print([(s.get_editor_property("animation").get_name(), s.get_editor_property("sample_value").x)
       for s in bs.get_editor_property("sample_data")])
