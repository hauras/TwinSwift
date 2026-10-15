"""태그 교체용 Niagara 생성. Unreal 공식 변환 API는 제작 시에만 사용한다."""
import unreal as u

fx=u.FXConverterUtilitiesLibrary
cat=u.ScriptExecutionCategory
tools=u.AssetToolsHelpers.get_asset_tools()
eal=u.EditorAssetLibrary
folder='/Game/VFX/Tag'

RING_CODE=r'''
float t=saturate(Age);
float2 p=(UV-0.5)*2;
float r=length(p);
float angle=atan2(p.y,p.x);
float radius=lerp(0.14,0.91,1-pow(1-t,2));
float main=exp(-pow((r-radius)/0.025,2));
float halo=0.22*exp(-pow((r-radius)/0.085,2));
float inner=0.28*exp(-pow((r-radius*0.78)/0.012,2));
float ticks=pow(saturate(cos(angle*12)),18)*exp(-pow((r-radius*0.86)/0.07,2));
float flash=exp(-r*16)*pow(1-t,5)*0.5;
return (main+halo+inner+ticks*0.28+flash)*pow(1-t,1.4)*smoothstep(0,0.06,t);
'''
GATHER_CODE=r'''
float t=saturate(Age);
float2 p=(UV-0.5)*2;
float r=length(p);
float a=atan2(p.y,p.x);
float radius=lerp(0.75,0.1,t*t);
float ring=exp(-pow((r-radius)/0.04,2));
float arcs=0.4+0.6*pow(saturate(cos(a*4+t*4)),3);
return ring*arcs*sin(t*3.14159265)*0.55;
'''
SPARK_CODE=r'''
float2 p=(UV-0.5)*2;
float diamond=abs(p.x)*0.8+abs(p.y)*1.4;
return pow(saturate(1-diamond),1.5)*pow(1-saturate(Age),1.8);
'''

def material(name,code):
    path=folder+'/'+name
    if eal.does_asset_exist(path):
        return u.load_asset(path)
    m=tools.create_asset(name,folder,u.Material,u.MaterialFactoryNew())
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided',True)
    m.set_editor_property('used_with_niagara_sprites',True)
    lib=u.MaterialEditingLibrary
    uv=lib.create_material_expression(m,u.MaterialExpressionTextureCoordinate,-700,0)
    age=lib.create_material_expression(m,u.MaterialExpressionParticleRelativeTime,-700,160)
    custom=lib.create_material_expression(m,u.MaterialExpressionCustom,-450,0)
    custom.set_editor_property('code',code)
    custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs=[]
    for input_name in ['UV','Age']:
        item=u.CustomInput()
        item.set_editor_property('input_name',input_name)
        inputs.append(item)
    custom.set_editor_property('inputs',inputs)
    lib.connect_material_expressions(uv,'',custom,'UV')
    lib.connect_material_expressions(age,'',custom,'Age')
    tint=lib.create_material_expression(m,u.MaterialExpressionParticleColor,-450,250)
    mul=lib.create_material_expression(m,u.MaterialExpressionMultiply,-150,0)
    lib.connect_material_expressions(custom,'',mul,'A')
    lib.connect_material_expressions(tint,'RGB',mul,'B')
    lib.connect_material_property(mul,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(m)
    assert eal.save_loaded_asset(m)
    return m

materials={k:material('M_TagSwap_'+k,code) for k,code in [('Ring',RING_CODE),('Gather',GATHER_CODE),('Spark',SPARK_CODE)]}

def script(path):
    return fx.create_script_context(u.CreateScriptContextArgs(fx.create_asset_data(path)))
def param(sc,name,value):
    assert sc.set_parameter(name,value), 'Missing module input '+name
def add_module(em,name,path,category):
    sc=script(path)
    em.add_module_script(name,sc,category)
    return sc
def random_velocity():
    sc=script('/Niagara/DynamicInputs/UniformRange/V2/RandomRangeVector.RandomRangeVector')
    param(sc,'Minimum',fx.create_script_input_vector(u.Vector(-180,-180,60)))
    param(sc,'Maximum',fx.create_script_input_vector(u.Vector(180,180,200)))
    return fx.create_script_input_dynamic(sc,u.NiagaraScriptInputType.VEC3)

def emitter(ctx,name,mat,color,size,lifetime,count,delay,ground):
    em=ctx.add_empty_emitter(name)
    em.set_local_space(False)
    em.set_sim_target(u.NiagaraSimTarget.CPU_SIM)
    state=add_module(em,'EmitterState','/Niagara/Modules/Emitter/EmitterState.EmitterState',cat.EMITTER_UPDATE)
    param(state,'Life Cycle Mode',fx.create_script_input_enum('/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode','Self'))
    param(state,'Loop Behavior',fx.create_script_input_enum('/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions','Once'))
    param(state,'Loop Duration',fx.create_script_input_float(0.32))
    param(state,'Inactive Response',fx.create_script_input_enum('/Niagara/Enums/ENiagaraInactiveMode.ENiagaraInactiveMode','Complete (Let Particles Finish then Kill Emitter)'))
    spawn=add_module(em,'SpawnBurst','/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous',cat.EMITTER_UPDATE)
    param(spawn,'Spawn Count',fx.create_script_input_int(count))
    param(spawn,'Spawn Time',fx.create_script_input_float(delay))
    # 변환 API는 Set Parameter를 모든 모듈보다 앞에 배치한다.
    # Initialize Particle을 뒤에 추가하면 색/수명/크기가 기본값으로 덮인다.
    # 필요한 초기 속성을 이 Set Parameter 단계에서 완전히 지정한다.
    em.set_parameter_directly('Particles.Position',fx.create_script_input_linked_parameter('Engine.Owner.Position',u.NiagaraScriptInputType.POSITION),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.Age',fx.create_script_input_float(0),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.NormalizedAge',fx.create_script_input_float(0),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.Alive',fx.create_script_input_bool(True),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.Mass',fx.create_script_input_float(1),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.Lifetime',fx.create_script_input_float(lifetime),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.Color',fx.create_script_input_linear_color(u.LinearColor(*color)),cat.PARTICLE_SPAWN)
    em.set_parameter_directly('Particles.SpriteSize',fx.create_script_input_vec2(u.Vector2D(size,size)),cat.PARTICLE_SPAWN)
    if ground:
        em.set_parameter_directly('Particles.SpriteFacing',fx.create_script_input_vector(u.Vector(0,0,1)),cat.PARTICLE_SPAWN)
    else:
        em.set_parameter_directly('Particles.Velocity',random_velocity(),cat.PARTICLE_SPAWN)
        add_module(em,'SolveForcesAndVelocity','/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity',cat.PARTICLE_UPDATE)
    add_module(em,'ParticleState','/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState',cat.PARTICLE_UPDATE)
    renderer=u.NiagaraSpriteRendererProperties()
    renderer.set_editor_property('material',mat)
    renderer.set_editor_property('facing_mode',u.NiagaraSpriteFacingMode.CUSTOM_FACING_VECTOR if ground else u.NiagaraSpriteFacingMode.FACE_CAMERA)
    em.add_renderer(name+'Renderer',renderer)
    em.set_enabled(True)
    return em

orange=(3.0,0.16,0.005,1)
blue=(0.015,0.25,3.0,1)
for role,old,new in [('ToMage',orange,blue),('ToWarrior',blue,orange)]:
    name='NS_TagSwap_'+role
    assert not eal.does_asset_exist(folder+'/'+name), 'Asset already exists '+name
    system=tools.create_asset(name,folder,u.NiagaraSystem,u.NiagaraSystemFactoryNew())
    ctx=fx.create_system_conversion_context(system)
    emitter(ctx,'Gather',materials['Gather'],old,175,0.1,1,0,True)
    emitter(ctx,'Pulse',materials['Ring'],new,190,0.22,1,0.04,True)
    emitter(ctx,'Sparks',materials['Spark'],new,10,0.22,12,0.04,False)
    ctx.finalize()
    ctx.cleanup()
    assert eal.save_loaded_asset(system)
    print('TAG_FX_CREATED',system.get_path_name())
print('TAG_FX_COMPLETE: two directions, three emitters each, procedural materials, no textures.')
