# 대검 메시(base_basic_shaded)에 칼날 시작·끝 소켓 추가
# ANS_WeaponTrace가 이 두 소켓 사이로 칼날 판정과 궤적 이펙트를 맞춤 (지금 궤적은 스켈레톤 TrailSocket + Timed Niagara Effect, 이 소켓은 판정만)
# 위치를 바꾸려면 숫자만 고치고 다시 실행, 또는 스태틱 메시 에디터에서 소켓을 직접 끌어도 됨
# 칼 모양 (원래 크기, 피벗=폼멜): 손잡이 10~40cm, 가드 40~60cm, 칼날 60~190cm
import unreal

eal = unreal.EditorAssetLibrary
SOCKETS = {
    "TrailStart": unreal.Vector(0, 0, 60),   # 칼날 시작 (가드 바로 위)
    "TrailEnd": unreal.Vector(0, 0, 188),    # 칼끝
}

sword = unreal.load_asset("/Game/Assets/Characters/Warrior/base_basic_shaded.base_basic_shaded")
for name, location in SOCKETS.items():
    socket = sword.find_socket(name)
    if not socket:
        socket = unreal.StaticMeshSocket(outer=sword)
        socket.set_editor_property("socket_name", name)
        sword.add_socket(socket)
    socket.set_editor_property("relative_location", location)
eal.save_loaded_asset(sword, only_if_is_dirty=False)

for name in SOCKETS:
    s = sword.find_socket(name)
    print(name, s.get_editor_property("relative_location"))
