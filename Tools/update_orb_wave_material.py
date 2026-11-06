import unreal as u
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError('PIE 종료 후 실행')
eal=u.EditorAssetLibrary
mel=u.MaterialEditingLibrary
path='/Game/Assets/VFX/Mage/ArcaneOrb/M_ArcaneOrb_Range'
mat=u.load_asset(path)
if not mat: raise RuntimeError('범위 재질 없음')
backup='/Game/Developers/Shared/Backups/BeforeOrbWave/M_ArcaneOrb_Range'
if not eal.does_asset_exist(backup):
    if not eal.duplicate_asset(path,backup): raise RuntimeError('재질 백업 실패')

# 기존 노드·색은 유지하고 모양 계산만 확장
expressions=u.MaterialEditingLibrary.get_material_expressions(mat) if hasattr(u.MaterialEditingLibrary,'get_material_expressions') else []
if not expressions:
    expressions=[x for x in u.ObjectIterator(u.MaterialExpression) if x.get_outer()==mat]
custom=next(x for x in expressions if isinstance(x,u.MaterialExpressionCustom))
custom.set_editor_property('code',r'''
float2 p=(UV-0.5)*2;
float r=length(p);
float edge=smoothstep(0.965,0.98,r)*(1-smoothstep(0.99,1.0,r));
float a=atan2(p.y,p.x);
float ticks=pow(saturate(cos(a*16)),24)*exp(-pow((r-0.88)/0.014,2));
float range=edge*(0.55+0.35*saturate(Charge))+ticks*0.3*saturate(Charge)+0.025*saturate(1-r);
float elapsed=max(0.0,Clock-StartTime);
float t=saturate(elapsed/0.30);
float radius=lerp(0.03,0.985,smoothstep(0.0,0.82,t));
float wave=exp(-pow((r-radius)/0.025,2))*(1-smoothstep(0.80,1.0,t));
float glow=0.2*exp(-pow((r-radius)/0.08,2))*(1-smoothstep(0.65,1.0,t));
float flash=edge*exp(-elapsed/0.055)*0.8;
return saturate(lerp(range,(wave+glow+flash)*Strength,saturate(BlastMode)));
''')
names=['UV','Charge','Clock','StartTime','Strength','BlastMode']
inputs=[]
for name in names:
    item=u.CustomInput()
    item.set_editor_property('input_name',name)
    inputs.append(item)
custom.set_editor_property('inputs',inputs)

params={str(x.get_editor_property('parameter_name')):x for x in expressions if isinstance(x,u.MaterialExpressionScalarParameter)}
for index,(name,default) in enumerate([('StartTime',0.0),('Strength',1.0),('BlastMode',0.0)]):
    node=params.get(name)
    if not node:
        node=mel.create_material_expression(mat,u.MaterialExpressionScalarParameter,-650,300+index*100)
        node.set_editor_property('parameter_name',name)
        node.set_editor_property('default_value',default)
    mel.connect_material_expressions(node,'',custom,name)
time=next((x for x in expressions if isinstance(x,u.MaterialExpressionTime)),None)
if not time: time=mel.create_material_expression(mat,u.MaterialExpressionTime,-650,650)
time.set_editor_property('ignore_pause',False)
mel.connect_material_expressions(time,'',custom,'Clock')
mel.recompile_material(mat)
eal.save_loaded_asset(mat,only_if_is_dirty=False)
print('ORB_WAVE_MATERIAL_SAVED: 바닥 원 + 0.3초 반경 파동')
