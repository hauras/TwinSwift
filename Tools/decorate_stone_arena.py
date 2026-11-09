import unreal as u

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if not world.get_path_name().startswith('/Game/Maps/L_StoneArena'):
    raise RuntimeError('L_StoneArena가 열려 있지 않음')
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError('PIE 종료 후 실행')

actors = u.get_editor_subsystem(u.EditorActorSubsystem)
# 다시 돌려도 겹치지 않게: 이전 장식·스크린샷 카메라 지움
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith(('Decor_', 'DecorShot_')):
        actors.destroy_actor(a)

P = '/Game/Fantastic_Dungeon_Pack/meshes/'
def mesh(path):
    name = path.split('/')[-1]
    m = u.load_asset(P + path + '.' + name)
    if not m:
        raise RuntimeError('메시 없음: ' + path)
    return m

count = 0
def place(label, path, x, y, z, yaw=0.0, scale=1.0, block=False):
    global count
    a = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(x, y, z), u.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    a.set_actor_label('Decor_' + label)
    a.set_folder_path('Arena/Decor')
    a.set_actor_scale3d(u.Vector(scale, scale, scale))
    c = a.static_mesh_component
    c.set_static_mesh(mesh(path))
    if block:
        # 캐릭터·탄은 막고 카메라는 통과 (벽 쪽 카메라가 당겨지지 않게)
        c.set_collision_profile_name('BlockAll')
        c.set_collision_response_to_channel(u.CollisionChannel.ECC_CAMERA, u.CollisionResponseType.ECR_IGNORE)
    else:
        c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    count += 1
    return a

def light(label, x, y, z, intensity, radius, color):
    global count
    a = actors.spawn_actor_from_class(u.PointLight, u.Vector(x, y, z))
    a.set_actor_label('Decor_' + label)
    a.set_folder_path('Arena/Decor')
    c = a.point_light_component
    c.set_editor_property('intensity', intensity)
    c.set_editor_property('attenuation_radius', radius)
    c.set_editor_property('light_color', color)
    c.set_editor_property('cast_shadows', False)
    count += 1

FIRE = u.Color(r=255, g=181, b=101, a=255)
EAST = 2700.0   # 동쪽 벽 안쪽 면
SOUTH = -1990.0  # 남쪽 벽 안쪽 면 (정면 카메라에서 배경이 되는 벽)

# ---- 기둥: 벽 속에 묻혀 있던 것을 반쯤 꺼냄 (평평한 벽을 칸으로 나눔) ----
# 카메라가 북쪽 위에서 남쪽을 봄 → 북쪽 기둥 5개를 배경인 남쪽 벽으로 옮김 (이름은 원래대로 north)
# 동쪽 가운데 기둥(y -10)은 보스 문 자리라 벽 속에 그대로 둠
for a in actors.get_all_level_actors():
    l = a.get_actor_label()
    if l.startswith('Arena_Column_north_'):
        loc = a.get_actor_location()
        a.modify(); a.set_actor_location(u.Vector(loc.x, SOUTH - 30, loc.z), False, False)
    elif l.startswith('Arena_Polish_Banner_'):
        # 원래 북쪽 벽에 있던 작은 깃발도 남쪽 벽으로 (벽을 향해 돌림)
        loc = a.get_actor_location()
        a.modify(); a.set_actor_location(u.Vector(loc.x, SOUTH + 20, loc.z), False, False)
        a.set_actor_rotation(u.Rotator(roll=0.0, pitch=0.0, yaw=180.0), False)
    elif l.startswith('Arena_Column_east_') and l != 'Arena_Column_east_2':
        loc = a.get_actor_location()
        a.modify(); a.set_actor_location(u.Vector(EAST + 30, loc.y, loc.z), False, False)

# ---- 동쪽 벽: 보스 문 ----
place('BossGate', 'modular/gateway/MOD_Gateway_Door_01_large', EAST - 26, -10, 0, yaw=90)
for side, sy in (('L', -1.0), ('R', 1.0)):
    place('GateBanner_' + side, 'props/fabrics/SM_PROP_flag_dungeon_05', EAST - 22, -10 + sy * 430, 120, yaw=90)
    bx, by = EAST - 150, -10 + sy * 310
    # 기존 구석 화로(0.56, 0.56, 1.07)보다 조금 크게, 불꽃 메시도 같은 방식
    b = place('GateBrazier_' + side, 'props/light/SM_PROP_brazier_dungeon_02', bx, by, 0, block=True)
    b.set_actor_scale3d(u.Vector(0.75, 0.75, 1.3))
    f = place('GateFlame_' + side, 'props/light/SM_PROP_fireplace_fire_dungeon_01', bx, by, 135)
    f.set_actor_scale3d(u.Vector(0.45, 0.6, 3.4))
    light('GateFire_' + side, bx, by, 230, 2200, 1200, FIRE)
    for i, (dx, dy) in enumerate(((-60, -50), (-80, 20), (-40, 70))):
        place('GateCandle_%s_%d' % (side, i), 'props/light/SM_PROP_candle_dungeon_05', bx + dx, by + sy * dy, 0)
# 보스 문에서 경기장 가운데까지 이어지는 양탄자 (바닥에 붙은 판, 충돌 없음)
for i in range(4):
    place('GateRug_%d' % i, 'props/fabrics/SM_PROP_rug_dungeon_01', EAST - 300 - i * 490, -10, 2, yaw=90)

# ---- 남쪽 벽(화면 배경): 기둥 사이에 매달린 쇠창살 우리 + 기둥 밑 잔해 ----
COLUMNS_X = (-880.0, -80.0, 720.0, 1520.0, 2320.0)
for i, cx in enumerate(COLUMNS_X):
    place('BackDebris_%d' % i, 'props/environment/SM_PROP_debris_stones_dungeon_01', cx + 110, SOUTH + 80, 0, yaw=i * 47.0, scale=1.5)
for i in range(len(COLUMNS_X) - 1):
    mx = (COLUMNS_X[i] + COLUMNS_X[i + 1]) * 0.5
    if i % 2 == 0:
        place('BackCage_%d' % i, 'props/traps/SM_PROP_cage_dungeon_01', mx, SOUTH + 150, 640, yaw=i * 30.0)
        place('BackCageChain_%d' % i, 'props/light/SM_PROP_chain_small_dungeon_01', mx, SOUTH + 150, 640)
    elif abs(mx - 1920.0) > 1.0:
        # 1920에는 원래 있던 작은 깃발(Arena_Polish_Banner_2)이 있음
        place('BackBanner_%d' % i, 'props/fabrics/SM_PROP_flag_dungeon_05', mx, SOUTH + 20, 120, yaw=180)
place('BackBones_0', 'props/environment/SM_PROP_skeleton_bonepile_dungeon_01', 320, SOUTH + 110, 0, yaw=20, scale=1.3)
place('BackBones_1', 'props/environment/SM_PROP_skeleton_bonepile_dungeon_01', 1920, SOUTH + 110, 0, yaw=-35, scale=1.3)

# ---- 카메라 쪽(북쪽) 벽·입구 복도 천장은 게임 중에 숨김 (충돌은 그대로 → 밖으로 못 나감) ----
# 카메라가 북쪽 벽 바깥 위에 있어서, 안 숨기면 벽 근처·복도에서 캐릭터를 가림
hidden = 0
for a in actors.get_all_level_actors():
    l = a.get_actor_label()
    o, e = a.get_actor_bounds(False)
    camera_side = (l.startswith('Indoor_Wall_north_')
        or (l.startswith('Indoor_Corner_') and o.y > 1900)
        or (l.startswith('Indoor_FoyerSide_') and o.y > 0)
        or l.startswith(('Indoor_FoyerCeiling', 'Indoor_FoyerRoof', 'Indoor_FoyerLintel')))
    if camera_side:
        a.modify()
        a.set_actor_hidden_in_game(True)
        hidden += 1
print('게임 중 숨김', hidden)

# ---- 동쪽 구석: 통·상자 ----
for k, sy in enumerate((1.0, -1.0)):
    cx, cy = EAST - 90, sy * 1650.0
    place('CornerBarrel_%d_0' % k, 'props/container/SM_PROP_barrel_dungeon_01', cx, cy, 0, scale=1.2, block=True)
    place('CornerBarrel_%d_1' % k, 'props/container/SM_PROP_barrel_dungeon_01', cx - 20, cy - sy * 110, 0, yaw=40, scale=1.2, block=True)
    place('CornerBox_%d' % k, 'props/container/SM_PROP_box_dungeon_01', cx - 130, cy + sy * 10, 0, yaw=15, scale=1.2, block=True)
    place('CornerDebris_%d' % k, 'props/environment/SM_PROP_debris_stones_dungeon_01', cx - 220, cy - sy * 150, 0, yaw=k * 90.0, scale=1.5)

# ---- 조명: 평평한 채움 빛을 낮추고 불빛을 키움 (불 주변이 따뜻하게 도드라지게) ----
for a in actors.get_all_level_actors():
    l = a.get_actor_label()
    if l.startswith('Indoor_Fill_'):
        a.modify(); a.rect_light_component.set_editor_property('intensity', 5000.0)
    elif l.startswith('Indoor_CeilingFill_'):
        a.modify(); a.rect_light_component.set_editor_property('intensity', 4000.0)
    elif l.startswith('Arena_Polish_FireLight_'):
        a.modify()
        c = a.point_light_component
        c.set_editor_property('intensity', 900.0)
        c.set_editor_property('attenuation_radius', 900.0)

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('장식', count, '저장')
