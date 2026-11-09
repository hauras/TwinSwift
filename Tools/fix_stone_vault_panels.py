import unreal as u
import math
ed=u.get_editor_subsystem(u.UnrealEditorSubsystem)
eas=u.get_editor_subsystem(u.EditorActorSubsystem)
assert not ed.get_game_world()
world=ed.get_editor_world()
assert world.get_path_name()=='/Game/Maps/L_StoneArena.L_StoneArena'
enc=next(a for a in eas.get_all_level_actors() if a.get_class().get_name()=='BP_Encounter_C')
c=enc.get_actor_location()
cx,cy=c.x,c.y
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith(('Indoor_Vault_','Indoor_RoofCollider_','Indoor_CeilingFill_')):
        eas.destroy_actor(a)
floor=u.load_asset('/Game/Fantastic_Dungeon_Pack/meshes/modular/floor/oneSided/MOD_Floor_01_O_straight_med')
cube=u.load_asset('/Engine/BasicShapes/Cube')
material=u.load_asset('/Game/Fantastic_Dungeon_Pack/materials/MI_MOD_Floor_01_v1')

def centered(mesh,label,center,scale,rotation,hidden=False):
    a=eas.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0),rotation)
    a.set_actor_label(label)
    a.set_folder_path('Arena/Indoor/'+('Collision' if hidden else 'Ceiling'))
    a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(u.Vector(*scale))
    # 엔진이 계산한 회전 후 경계 중심을 사용해 피벗과 회전 부호를 추측하지 않는다.
    origin,extent=a.get_actor_bounds(False)
    a.set_actor_location(u.Vector(*center)-origin,False,False)
    a.static_mesh_component.set_collision_profile_name('BlockAll' if hidden else 'NoCollision')
    if hidden:
        a.set_actor_hidden_in_game(True)
        a.static_mesh_component.set_editor_property('cast_shadow',False)
    else:
        a.static_mesh_component.set_material(0,material)
    return a

radius=3000.0
span=2200.0
alpha=math.asin(span/radius)
base=1800-math.sqrt(radius*radius-span*span)
for band in range(12):
    p0=-alpha+2*alpha*band/12
    p1=-alpha+2*alpha*(band+1)/12
    mid=(p0+p1)*.5
    y0,y1=radius*math.sin(p0),radius*math.sin(p1)
    z0,z1=base+radius*math.cos(p0),base+radius*math.cos(p1)
    my,mz=(y0+y1)*.5,(z0+z1)*.5
    chord=math.hypot(y1-y0,z1-z0)+10
    normal=u.Vector(0,-math.sin(mid),-math.cos(mid))
    rotation=u.MathLibrary.make_rot_from_zx(normal,u.Vector(1,0,0))
    for ix in range(11):
        centered(floor,'Indoor_Vault_%02d_%02d'%(band,ix),(cx-2000+400*ix,cy+my,mz),(1.0025,chord/400,1),rotation)
    centered(cube,'Indoor_RoofCollider_%02d'%band,(cx,cy+my,mz+25),(44.2,chord/100,.5),rotation,True)

for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith('Indoor_Fill_'):
        a.light_component.set_editor_property('intensity',8000)
for i,x in enumerate([-1000,1000]):
    a=eas.spawn_actor_from_class(u.RectLight,u.Vector(cx+x,cy,1700),u.Rotator(pitch=90))
    a.set_actor_label('Indoor_CeilingFill_%d'%i)
    a.set_folder_path('Arena/Indoor/Lighting')
    comp=a.light_component
    for prop,value in [('intensity',6000),('attenuation_radius',1800),('source_width',1000),('source_height',1000),('cast_shadows',False)]:
        comp.set_editor_property(prop,value)
    comp.set_light_color(u.LinearColor(.7,.78,1,1))
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/Maps/L_StoneArena')
print('STONE_VAULT_PANELS_ALIGNED')
