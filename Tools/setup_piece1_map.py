# 단계 1 임시 플레이어(엔진 튜토리얼 마네킹) + 테스트 맵 L_Dev
# 믹사모를 받으면 BP_Player의 메시·애님만 바꾸면 됨
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

# ---------- BP_Player ----------
path = "/Game/Blueprints/BP_Player"
if eal.does_asset_exist(path):
    bp_player = eal.load_asset(path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.PlayerCharacter)
    bp_player = asset_tools.create_asset("BP_Player", "/Game/Blueprints", unreal.Blueprint, factory)

cdo = unreal.get_default_object(bp_player.generated_class())
mesh = cdo.get_editor_property("mesh")
mesh.set_editor_property("skeletal_mesh_asset", unreal.load_asset("/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP"))
# 캡슐 바닥에 발을 맞추고, 메시 앞(+Y)을 액터 앞(+X)으로 돌림
mesh.set_editor_property("relative_location", unreal.Vector(0, 0, -90))
mesh.set_editor_property("relative_rotation", unreal.Rotator(roll=0, pitch=0, yaw=-90))
# 임시: 애님 BP 대신 Idle 하나만 재생 (믹사모 받은 뒤 ABP_Player로 교체)
mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
anim_data = mesh.get_editor_property("animation_data")
anim_data.set_editor_property("anim_to_play", unreal.load_asset("/Engine/Tutorial/SubEditors/TutorialAssets/Character/Tutorial_Idle"))
anim_data.set_editor_property("saved_looping", True)
anim_data.set_editor_property("saved_playing", True)
mesh.set_editor_property("animation_data", anim_data)

unreal.BlueprintEditorLibrary.compile_blueprint(bp_player)
eal.save_loaded_asset(bp_player, only_if_is_dirty=False)

bp_gm = eal.load_asset("/Game/Blueprints/BP_GameMode")
gm_cdo = unreal.get_default_object(bp_gm.generated_class())
gm_cdo.set_editor_property("default_pawn_class", bp_player.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(bp_gm)
eal.save_loaded_asset(bp_gm, only_if_is_dirty=False)
print("BP_Player 완료")

# ---------- L_Dev ----------
map_path = "/Game/Maps/L_Dev"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if eal.does_asset_exist(map_path):
    les.load_level(map_path)
    print("L_Dev 이미 있음 → 열기만 함")
else:
    les.new_level(map_path)

    def spawn(cls, loc, rot=unreal.Rotator(), label=None):
        a = eas.spawn_actor_from_class(cls, loc, rot)
        if label:
            a.set_actor_label(label)
        return a

    def shape(mesh_name, loc, scale, label):
        a = spawn(unreal.StaticMeshActor, loc, label=label)
        a.static_mesh_component.set_static_mesh(unreal.load_asset(f"/Engine/BasicShapes/{mesh_name}"))
        a.set_actor_scale3d(scale)
        return a

    # 바닥 60m x 60m
    shape("Plane", unreal.Vector(0, 0, 0), unreal.Vector(60, 60, 1), "Floor")

    # 거리감 확인용 기둥: 5m 간격 격자 (쿼터뷰에서 가려짐, 3인칭에서 시야 확인)
    for x in range(-2, 3):
        for y in range(-2, 3):
            if x == 0 and y == 0:
                continue
            shape("Cube", unreal.Vector(x * 1000, y * 1000, 150), unreal.Vector(1, 1, 3), f"Pillar_{x}_{y}")

    # 보스 크기 비교용 (4m 높이)
    shape("Cylinder", unreal.Vector(2500, 0, 200), unreal.Vector(3, 3, 4), "BossSize")

    sun = spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(roll=0, pitch=-50, yaw=30), "Sun")
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    spawn(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), label="SkyAtmosphere")
    sky = spawn(unreal.SkyLight, unreal.Vector(0, 0, 500), label="SkyLight")
    sky.light_component.set_editor_property("real_time_capture", True)
    spawn(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), label="Fog")
    spawn(unreal.PlayerStart, unreal.Vector(0, 0, 100), label="PlayerStart")

    # 이 맵에서는 BP_GameMode 사용
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", bp_gm.generated_class())

    les.save_current_level()
    print("L_Dev 만듦")
