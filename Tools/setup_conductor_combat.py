import unreal as u
E=u.EditorAssetLibrary
T=u.AssetToolsHelpers.get_asset_tools()
BASE='/Game/Assets/Characters/Boss/Conductor'
PARENT=u.load_class(None,'/Script/TwinSwift.SGA_ConductorPattern')
if not PARENT:
    raise RuntimeError('SGA_ConductorPattern이 포함된 TwinSwiftEditor 빌드가 필요합니다.')
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError('PIE를 종료한 뒤 적용하세요.')
def tags(*names):
    c=u.GameplayTagContainer()
    c.import_text('(GameplayTags=('+','.join('(TagName="'+n+'")' for n in names)+'))')
    return c
def tag(name):
    t=u.GameplayTag();t.import_text('(TagName="'+name+'")');return t
def blueprint(path,parent):
    if E.does_asset_exist(path):return u.load_asset(path)
    folder,name=path.rsplit('/',1)
    f=u.BlueprintFactory();f.set_editor_property('parent_class',parent)
    return T.create_asset(name,folder,u.Blueprint,f)
specs=[
    ('Ring','1HCastSpell_01',u.ConductorPattern.RING,16,2,.35,False,3.0),
    ('Fan','2HCastSpell_01',u.ConductorPattern.AIMED_FAN,7,2,.35,True,4.0),
    ('Spiral','2HMagicAreaAttack_01',u.ConductorPattern.SPIRAL,6,8,.15,False,5.0),
]
abilities=[];patterns=[]
for name,anim,kind,count,volleys,interval,blue,cooldown in specs:
    mp=BASE+'/Montages/AM_Conductor_'+name
    if E.does_asset_exist(mp):montage=u.load_asset(mp)
    else:
        f=u.AnimMontageFactory();f.set_editor_property('source_animation',u.load_asset(BASE+'/Animations/A_Conductor_'+anim))
        montage=T.create_asset(mp.rsplit('/',1)[1],BASE+'/Montages',u.AnimMontage,f)
    E.save_loaded_asset(montage)
    ga=blueprint('/Game/Abilities/Boss/GA_Conductor_'+name,PARENT)
    u.BlueprintEditorLibrary.compile_blueprint(ga)
    c=u.get_default_object(ga.generated_class())
    props=dict(pattern=kind,cast_montage=montage,bullets_per_volley=count,volley_count=volleys,
        first_shot_time=.65,volley_interval=interval,bullet_speed=500.0,fan_angle=70.0,
        spiral_step=12.0,damage_multiplier=1.0,b_blue=blue,cooldown_duration=cooldown)
    # Unreal Python은 bBlue를 blue로 노출한다.
    props.pop('b_blue');props['blue']=blue
    for k,v in props.items():c.set_editor_property(k,v)
    c.set_editor_property('cooldown_tags',tags('Cooldown.Boss.Conductor.'+name))
    assigned=False
    for field in ['asset_tags','ability_tags']:
        try:c.set_editor_property(field,tags('Ability.Boss.Conductor.'+name));assigned=True;break
        except Exception:pass
    assert assigned,'능력 태그 설정 실패'
    u.BlueprintEditorLibrary.compile_blueprint(ga);E.save_loaded_asset(ga)
    abilities.append(ga.generated_class())
    p=u.SBossPattern()
    for k,v in dict(ability_tag=tag('Ability.Boss.Conductor.'+name),min_range=0.0,max_range=6000.0,preferred_range=1000.0,weight=1.0).items():p.set_editor_property(k,v)
    patterns.append(p)
    print('PATTERN_READY',name,montage.get_play_length(),count,volleys)
bbpath='/Game/Blueprints/AI/Conductor/BB_Conductor'
if E.does_asset_exist(bbpath):bb=u.load_asset(bbpath)
else:bb=T.create_asset('BB_Conductor','/Game/Blueprints/AI/Conductor',u.BlackboardData,u.BlackboardDataFactory())
# BT는 에디터에서 손으로 만든 것 (ROOT > Selector[Find Nearest Player] > Sequence[Activate Boss Pattern → Wait 1초])
# 코드로 실행용 노드만 넣으면 저장할 때 에디터 그래프로 다시 만들어지며 지워짐 → 그래프는 손으로
btpath='/Game/Blueprints/AI/Conductor/BT_Conductor'
assert E.does_asset_exist(btpath),'BT_Conductor를 에디터에서 먼저 만들 것'
bt=u.load_asset(btpath)
E.save_loaded_asset(bb)
bp=u.load_asset('/Game/Blueprints/Boss/BP_Boss_Conductor')
c=u.get_default_object(bp.generated_class())
c.set_editor_property('startup_abilities',abilities)
c.set_editor_property('behavior_tree',bt)
c.get_editor_property('pattern_component').set_editor_property('patterns',patterns)
u.BlueprintEditorLibrary.compile_blueprint(bp);E.save_loaded_asset(bp)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
allactors=actors.get_all_level_actors()
boss=next(a for a in allactors if a.get_class()==bp.generated_class())
enc=next((a for a in allactors if isinstance(a,u.SBossEncounter) and a.get_editor_property('boss')==boss),None)
if not enc:
    encclass=u.load_asset('/Game/Blueprints/Boss/BP_Encounter').generated_class()
    loc=boss.get_actor_location();loc.z-=100.0
    enc=actors.spawn_actor_from_class(encclass,loc)
    enc.set_actor_label('Encounter_Conductor')
enc.set_editor_property('boss',boss)
enc.set_editor_property('play_intro',False)
enc.get_editor_property('trigger').set_box_extent(u.Vector(650,650,350))
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('CONDUCTOR_COMBAT_CONNECTED',boss.get_path_name(),enc.get_path_name())
