"""석조 경기장을 닫힌 보스 홀로 변경. 실행 중인 편집기에서만 적용."""
import unreal as u
import math

ed=u.get_editor_subsystem(u.UnrealEditorSubsystem)
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
eas=u.get_editor_subsystem(u.EditorActorSubsystem)
eal=u.EditorAssetLibrary
assert not ed.get_game_world(), 'PIE 중에는 수정하지 않는다'
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages(), '미저장 맵을 먼저 보존해야 함'
target='/Game/Maps/L_StoneArena'
assert ed.get_editor_world().get_path_name()==target+'.L_StoneArena'
backup='/Game/Developers/Shared/Backups/Maps/L_StoneArena_Open_20261106'
if not eal.does_asset_exist(backup):
    assert u.EditorLoadingAndSavingUtils.save_map(ed.get_editor_world(),backup)
    assert les.load_level(target)
world=ed.get_editor_world()
assert world.get_path_name()==target+'.L_StoneArena'
all_actors=eas.get_all_level_actors()
enc=next(a for a in all_actors if a.get_class().get_name()=='BP_Encounter_C')
center=enc.get_actor_location()
cx,cy=center.x,center.y
root='/Game/Fantastic_Dungeon_Pack/meshes/'
meshes={
 'wall':u.load_asset(root+'modular/wall/pivotMiddle/MOD_Wall_01_M_straight_large'),
 'floor':u.load_asset(root+'modular/floor/oneSided/MOD_Floor_01_O_straight_med'),
 'arch':u.load_asset(root+'modular/gateway/MOD_Gateway_Arc_01'),
 'door_l':u.load_asset(root+'modular/gateway/MOD_Gateway_Door_01_large_door_L'),
 'door_r':u.load_asset(root+'modular/gateway/MOD_Gateway_Door_01_large_door_R'),
 'cube':u.load_asset('/Engine/BasicShapes/Cube')}
assert all(meshes.values())
for a in all_actors:
    label=a.get_actor_label()
    if label.startswith('Indoor_') or label.startswith('Arena_Wall_') or label.startswith('Arena_Polish_Gate_') or label.startswith('Arena_Polish_EntryWall_'):
        eas.destroy_actor(a)

def mesh_actor(key,label,loc,scale=(1,1,1),rot=None,collision=True,folder='Architecture'):
    a=eas.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*loc),rot or u.Rotator())
    a.set_actor_label('Indoor_'+label)
    a.set_folder_path('Arena/Indoor/'+folder)
    a.static_mesh_component.set_static_mesh(meshes[key])
    a.static_mesh_component.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    a.set_actor_scale3d(u.Vector(*scale))
    return a

def fit(key,label,base,size,yaw=0,collision=True,folder='Architecture'):
    if key=='wall' and size[2]>600:
        last=None
        z=0.0
        layer=0
        while z<size[2]-.01:
            h=min(570.0,size[2]-z)
            last=fit(key,label+'_L%d'%layer,(base[0],base[1],base[2]+z),(size[0],size[1],h),yaw,collision,folder)
            z+=h
            layer+=1
        return last
    box=meshes[key].get_bounding_box()
    lo,hi=box.min,box.max
    ext=[hi.x-lo.x,hi.y-lo.y,hi.z-lo.z]
    scale=[size[i]/ext[i] if ext[i]>.01 else 1 for i in range(3)]
    ox=(lo.x+hi.x)*.5*scale[0]
    oy=(lo.y+hi.y)*.5*scale[1]
    angle=math.radians(yaw)
    pos=(base[0]-ox*math.cos(angle)+oy*math.sin(angle),base[1]-ox*math.sin(angle)-oy*math.cos(angle),base[2]-lo.z*scale[2])
    return mesh_actor(key,label,pos,scale,u.Rotator(yaw=yaw),collision,folder)

# 카메라 쪽도 생략하지 않고 막는다. 서쪽 입구만 높이 7m까지 통과 가능.
for side in ['east','west','north','south']:
    for i in range(10):
        offset=-1800+400*i
        height=2800 if side in ['east','west'] else 2020
        bottom=0
        if side=='west' and i in [4,5]:
            bottom=700
            height-=700
        if side in ['east','west']:
            pos=(cx+(2050 if side=='east' else -2050),cy+offset,bottom)
            yaw=90
        else:
            pos=(cx+offset,cy+(2050 if side=='north' else -2050),bottom)
            yaw=0
        fit('wall','Wall_%s_%d'%(side,i),pos,(400,140,height),yaw)

# 외벽 연결부와 네 모서리를 막는다.
for x in [-2050,2050]:
    for y in [-2050,2050]:
        fit('wall','Corner_%d_%d'%(x,y),(cx+x,cy+y,0),(180,180,2800))

# 배럴 볼트: 12개의 완만한 면으로 둥근 천장을 구성한다.
radius=3000.0
span=2200.0
edge_height=1800.0
angle_max=math.asin(span/radius)
zbase=edge_height-math.sqrt(radius*radius-span*span)
for band in range(12):
    a0=-angle_max+2*angle_max*band/12
    a1=-angle_max+2*angle_max*(band+1)/12
    y0,y1=radius*math.sin(a0),radius*math.sin(a1)
    z0,z1=zbase+radius*math.cos(a0),zbase+radius*math.cos(a1)
    my,mz=(y0+y1)*.5,(z0+z1)*.5
    chord=math.hypot(y1-y0,z1-z0)+3
    mid=(a0+a1)*.5
    roll=math.pi-mid
    rotation=u.Rotator(roll=math.degrees(roll))
    for ix in range(11):
        # 바닥 모듈의 앞면을 아래로 향하게 해서 실제 실내 천장으로 쓴다.
        loc=(cx-2200+ix*400,cy+my-.5*chord*math.cos(roll),mz-.5*chord*math.sin(roll))
        tile=mesh_actor('floor','Vault_%02d_%02d'%(band,ix),loc,(1,chord/400,1),rotation,False,'Ceiling')
        tile.static_mesh_component.set_material(0,u.load_asset('/Game/Fantastic_Dungeon_Pack/materials/MI_MOD_Floor_01_v1'))
    slab=mesh_actor('cube','RoofCollider_%02d'%band,(cx,cy+my,mz+25),(44,chord/100,.5),rotation,True,'Collision')
    slab.set_actor_hidden_in_game(True)
    slab.static_mesh_component.set_editor_property('cast_shadow',False)

# 천장 리브와 벽기둥을 연결해 긴 홀의 형태를 읽게 한다.
for i,offset in enumerate([-1950,-1000,0,1000,1950]):
    fit('arch','VaultRib_%d'%i,(cx+offset,cy,1720),(4380,110,1040),90,False,'Ceiling')
for a in eas.get_all_level_actors():
    label=a.get_actor_label()
    if label.startswith('Arena_Column_'):
        a.set_actor_scale3d(u.Vector(2.0,2.0,2000/390.0))

# 입구 복도와 뒤쪽 문도 닫아 화면에 빈 바깥 공간이 보이지 않게 한다.
for side in [-1,1]:
    fit('wall','FoyerSide_%d'%side,(cx-2600,cy+460*side,0),(1200,140,1800))
fit('wall','FoyerBack',(cx-3200,cy,0),(920,140,1800),90)
fit('wall','FoyerLintel',(cx-2050,cy,700),(800,140,2100),90)
fit('arch','EntryArch',(cx-2050,cy,0),(920,160,720),90,False)
fit('door_l','EntryDoorLeft',(cx-3110,cy-200,0),(400,100,700),90)
fit('door_r','EntryDoorRight',(cx-3110,cy+200,0),(400,100,700),90)
# 복도 천장은 평평하고 플레이 카메라보다 높다.
fit('cube','FoyerRoof',(cx-2600,cy,1800),(1200,1060,80))
for ix in range(3):
    for iy in range(3):
        mesh_actor('floor','FoyerCeiling_%d_%d'%(ix,iy),(cx-3200+400*ix,cy+600-400*iy,1795),(1,1,1),u.Rotator(roll=180),False,'Ceiling')
fit('cube','FoyerFloorExtension',(cx-3100,cy,-100),(400,800,100))

# 원본 벽 모듈의 단순 충돌 유무와 무관하게 카메라·캐릭터를 실내에 유지한다.
shell_boxes=[
    ('East',(cx+2050,cy,0),(140,4100,2800)),
    ('WestN',(cx-2050,cy+1250,0),(140,1700,2800)),
    ('WestS',(cx-2050,cy-1250,0),(140,1700,2800)),
    ('WestTop',(cx-2050,cy,700),(140,800,2100)),
    ('North',(cx,cy+2050,0),(4100,140,2020)),
    ('South',(cx,cy-2050,0),(4100,140,2020)),
    ('FoyerN',(cx-2600,cy+460,0),(1200,140,1800)),
    ('FoyerS',(cx-2600,cy-460,0),(1200,140,1800)),
    ('FoyerEnd',(cx-3200,cy,0),(140,1060,1800))]
for label,pos,size in shell_boxes:
    a=fit('cube','ShellCollider_'+label,pos,size,folder='Collision')
    a.set_actor_hidden_in_game(True)
    a.static_mesh_component.set_editor_property('cast_shadow',False)

# 외부 태양광 대신 그림자를 끈 실내 보조광 4개를 사용한다.
for a in eas.get_all_level_actors():
    if isinstance(a,u.DirectionalLight):
        a.light_component.set_editor_property('intensity',0.0)
    if isinstance(a,u.SkyLight):
        a.light_component.set_editor_property('intensity',.35)
        a.light_component.set_light_color(u.LinearColor(.7,.8,1,1))
    if isinstance(a,u.ExponentialHeightFog):
        a.get_components_by_class(u.ExponentialHeightFogComponent)[0].set_editor_property('fog_density',.001)
for i,(x,y) in enumerate([(-1000,-1000),(-1000,1000),(1000,-1000),(1000,1000)]):
    a=eas.spawn_actor_from_class(u.RectLight,u.Vector(cx+x,cy+y,1550),u.Rotator(pitch=-90))
    a.set_actor_label('Indoor_Fill_%d'%i)
    a.set_folder_path('Arena/Indoor/Lighting')
    comp=a.light_component
    comp.set_editor_property('intensity',18000)
    comp.set_editor_property('attenuation_radius',2400)
    comp.set_editor_property('source_width',900)
    comp.set_editor_property('source_height',900)
    comp.set_editor_property('cast_shadows',False)
    comp.set_light_color(u.LinearColor(.65,.78,1,1))
foyer=eas.spawn_actor_from_class(u.PointLight,u.Vector(cx-2600,cy,1000))
foyer.set_actor_label('Indoor_FoyerLight')
foyer.set_folder_path('Arena/Indoor/Lighting')
foyer.light_component.set_editor_property('intensity',4500)
foyer.light_component.set_editor_property('attenuation_radius',1500)
foyer.light_component.set_editor_property('cast_shadows',False)
foyer.light_component.set_light_color(u.LinearColor(1,.65,.35,1))

u.SystemLibrary.execute_console_command(world,'RebuildNavigation')
assert u.EditorLoadingAndSavingUtils.save_map(world,target)
# 실내 시점에서 마무리. 플레이 카메라와 천장은 그대로 검사한다.
ed.set_level_viewport_camera_info(u.Vector(cx-1300,cy-1300,1150),u.Rotator(pitch=-32,yaw=45))
les.editor_set_game_view(True)
print('INDOOR_ARENA_SAVED',target,'vault edge 18m / crown',round((zbase+radius)/100,2),'m')
