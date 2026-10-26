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
# 패턴 = 데이터 에셋 (DA_Conductor_*). 지시 한 줄: 첫 발 시각, 탄 수, 모양, 반복 수·간격, 반복마다 돌릴 각도, 색
# 방사형: 고리 16발 2번, 두 번째를 반 칸(11.25도) 돌려 앞 고리의 틈을 막음 / 부채꼴: 파랑 7발 70도 2번 / 나선: 고리 6발 8번, 12도씩
PATTERN_DIR='/Game/Abilities/Boss/Patterns'
specs=[
    ('Ring','1HCastSpell_01',3.0,[dict(fire_time=.65,bullet_count=16,shape=u.BulletShape.RING,repeat_count=2,repeat_interval=.35,angle_step_per_repeat=360.0/16/2,color=u.BulletColor.ORANGE)]),
    ('Fan','2HCastSpell_01',4.0,[dict(fire_time=.65,bullet_count=7,shape=u.BulletShape.FAN,spread=70.0,repeat_count=2,repeat_interval=.35,color=u.BulletColor.BLUE)]),
    ('Spiral','2HMagicAreaAttack_01',5.0,[dict(fire_time=.65,bullet_count=6,shape=u.BulletShape.RING,repeat_count=8,repeat_interval=.15,angle_step_per_repeat=12.0,color=u.BulletColor.ORANGE)]),
]
def pattern_data(name,events):
    path=PATTERN_DIR+'/DA_Conductor_'+name
    if E.does_asset_exist(path):da=u.load_asset(path)
    else:
        f=u.DataAssetFactory();f.set_editor_property('data_asset_class',u.SBulletPatternData)
        da=T.create_asset('DA_Conductor_'+name,PATTERN_DIR,u.SBulletPatternData,f)
    rows=[]
    for e in events:
        row=u.SBulletPatternEvent()
        for k,v in dict(dict(speed=500.0,damage_multiplier=1.0),**e).items():row.set_editor_property(k,v)
        rows.append(row)
    da.set_editor_property('events',rows)
    E.save_loaded_asset(da,only_if_is_dirty=False)
    return da
abilities=[];patterns=[]
for name,anim,cooldown,events in specs:
    mp=BASE+'/Montages/AM_Conductor_'+name
    if E.does_asset_exist(mp):montage=u.load_asset(mp)
    else:
        f=u.AnimMontageFactory();f.set_editor_property('source_animation',u.load_asset(BASE+'/Animations/A_Conductor_'+anim))
        montage=T.create_asset(mp.rsplit('/',1)[1],BASE+'/Montages',u.AnimMontage,f)
    E.save_loaded_asset(montage)
    da=pattern_data(name,events)
    ga=blueprint('/Game/Abilities/Boss/GA_Conductor_'+name,PARENT)
    u.BlueprintEditorLibrary.compile_blueprint(ga)
    c=u.get_default_object(ga.generated_class())
    for k,v in dict(cast_montage=montage,pattern_data=da,cooldown_duration=cooldown).items():c.set_editor_property(k,v)
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
    print('PATTERN_READY',name,'몽타주',round(montage.get_play_length(),2),'마지막 발사',round(da.get_last_shot_time(),2) if hasattr(da,'get_last_shot_time') else '-')
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
# 에디터 파이썬에 에셋을 담은 변수가 남으면 에셋을 붙잡아 지우기·이름 바꾸기가 막힘 → 끝에서 비움
for _name in [k for k, v in list(globals().items()) if isinstance(v, (u.Object, list))]:
    del globals()[_name]
