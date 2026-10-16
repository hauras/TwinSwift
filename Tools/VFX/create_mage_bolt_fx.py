"""마탄 발사/비행/명중 Niagara. 초기화 덮어쓰기 없이 입자 속성을 직접 지정한다."""
import ast
from pathlib import Path
import unreal as u

fx=u.FXConverterUtilitiesLibrary
cat=u.ScriptExecutionCategory
tools=u.AssetToolsHelpers.get_asset_tools()
eal=u.EditorAssetLibrary
folder='/Game/VFX/Mage'
# 검증된 제작 도우미만 재사용하며, 교체 이펙트 생성 본문은 실행하지 않는다.
source=Path('D:/Unreal Projects/TwinSwift/Tools/VFX/create_tag_swap_fx.py').read_text(encoding='utf-8')
nodes=[n for n in ast.parse(source).body if isinstance(n,ast.FunctionDef) and n.name in ['material','script','param','add_module','random_velocity']]
exec(compile(ast.fix_missing_locations(ast.Module(body=nodes,type_ignores=[])),'<Niagara helpers>','exec'))

ORB=r'''
float r=length((UV-0.5)*2);
float rim=exp(-pow((r-0.38)/0.16,2))*0.5;
float core=pow(saturate(1-r/0.55),1.8);
return (core+rim+0.12*exp(-r*r*5))*pow(1-saturate(Age),0.5);
'''
TAIL=r'''
float r=length((UV-0.5)*2);
return pow(saturate(1-r),2)*pow(1-saturate(Age),2);
'''
RING=r'''
float t=saturate(Age);
float2 p=(UV-0.5)*2;
float r=length(p);
float radius=lerp(0.08,0.85,t);
return (exp(-pow((r-radius)/0.04,2))+exp(-r*12)*0.5)*pow(1-t,1.6);
'''
SPARK=r'''
float2 p=(UV-0.5)*2;
return pow(saturate(1-abs(p.x)-abs(p.y)*0.6),2)*pow(1-saturate(Age),1.5);
'''
mats={k:material('M_MageBolt_'+k,v) for k,v in [('Orb',ORB),('Tail',TAIL),('Ring',RING),('Spark',SPARK)]}
blue=(0.015,0.3,3.0,1)
pale=(1.3,2.0,3.0,1)

def emit(ctx,name,kind,color,size,life,count=1,rate=0,velocity=False):
    em=ctx.add_empty_emitter(name)
    em.set_local_space(False)
    em.set_sim_target(u.NiagaraSimTarget.CPU_SIM)
    state=add_module(em,'EmitterState','/Niagara/Modules/Emitter/EmitterState.EmitterState',cat.EMITTER_UPDATE)
    param(state,'Life Cycle Mode',fx.create_script_input_enum('/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode','Self'))
    param(state,'Loop Behavior',fx.create_script_input_enum('/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions','Infinite' if rate else 'Once'))
    param(state,'Loop Duration',fx.create_script_input_float(0.2))
    param(state,'Inactive Response',fx.create_script_input_enum('/Niagara/Enums/ENiagaraInactiveMode.ENiagaraInactiveMode','Complete (Let Particles Finish then Kill Emitter)'))
    if rate:
        spawn=add_module(em,'SpawnRate','/Niagara/Modules/Emitter/SpawnRate.SpawnRate',cat.EMITTER_UPDATE)
        param(spawn,'SpawnRate',fx.create_script_input_float(rate))
    else:
        spawn=add_module(em,'SpawnBurst','/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous',cat.EMITTER_UPDATE)
        param(spawn,'Spawn Count',fx.create_script_input_int(count))
        param(spawn,'Spawn Time',fx.create_script_input_float(0))
    values={
        'Particles.Position':fx.create_script_input_linked_parameter('Engine.Owner.Position',u.NiagaraScriptInputType.POSITION),
        'Particles.Age':fx.create_script_input_float(0),
        'Particles.NormalizedAge':fx.create_script_input_float(0),
        'Particles.Alive':fx.create_script_input_bool(True),
        'Particles.Mass':fx.create_script_input_float(1),
        'Particles.Lifetime':fx.create_script_input_float(life),
        'Particles.Color':fx.create_script_input_linear_color(u.LinearColor(*color)),
        'Particles.SpriteSize':fx.create_script_input_vec2(u.Vector2D(size,size)),
    }
    if velocity: values['Particles.Velocity']=random_velocity()
    for key,value in values.items(): em.set_parameter_directly(key,value,cat.PARTICLE_SPAWN)
    if velocity: add_module(em,'Solve','/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity',cat.PARTICLE_UPDATE)
    add_module(em,'ParticleState','/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState',cat.PARTICLE_UPDATE)
    renderer=u.NiagaraSpriteRendererProperties()
    renderer.set_editor_property('material',mats[kind])
    renderer.set_editor_property('facing_mode',u.NiagaraSpriteFacingMode.FACE_CAMERA)
    em.add_renderer(name+'Renderer',renderer)
    em.set_enabled(True)

definitions={
    'Flight': [('Core','Orb',pale,24,0.018,1,140,False),('Halo','Orb',blue,40,0.025,1,90,False),('Tail','Tail',blue,23,0.14,1,90,False)],
    'Launch': [('Flash','Orb',pale,38,0.08),('Ring','Ring',blue,65,0.12),('Flecks','Spark',blue,8,0.12,6,0,True)],
    'Impact': [('Flash','Orb',pale,46,0.08),('Ring','Ring',blue,85,0.16),('Flecks','Spark',blue,10,0.18,10,0,True)]
}
for label,emitters in definitions.items():
    name='NS_MageBolt_'+label
    assert not eal.does_asset_exist(folder+'/'+name), 'Refusing to overwrite '+name
    system=tools.create_asset(name,folder,u.NiagaraSystem,u.NiagaraSystemFactoryNew())
    ctx=fx.create_system_conversion_context(system)
    for args in emitters: emit(ctx,*args)
    ctx.finalize()
    ctx.cleanup()
    assert eal.save_loaded_asset(system)
    print('MAGE_BOLT_FX_CREATED',system.get_path_name())
print('MAGE_BOLT_FX_COMPLETE')
