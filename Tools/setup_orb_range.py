"""범위 원 재질 생성. 빌드 반영 후 BP_ArcaneOrb의 새 컴포넌트와 피격 효과 연결."""
import unreal as u
eal=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
ed=u.get_editor_subsystem(u.UnrealEditorSubsystem)
if ed and ed.get_game_world():
    raise RuntimeError('PIE 종료 필요')
path='/Game/Assets/VFX/Mage/ArcaneOrb/M_ArcaneOrb_Range'
mat=u.load_asset(path)
if not mat:
    folder,name=path.rsplit('/',1)
    mat=tools.create_asset(name,folder,u.Material,u.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    mel=u.MaterialEditingLibrary
    uv=mel.create_material_expression(mat,u.MaterialExpressionTextureCoordinate,-600,0)
    charge=mel.create_material_expression(mat,u.MaterialExpressionScalarParameter,-600,180)
    charge.set_editor_property('parameter_name','Charge')
    charge.set_editor_property('default_value',0.0)
    shape=mel.create_material_expression(mat,u.MaterialExpressionCustom,-350,0)
    shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    shape.set_editor_property('code',r'''
float2 p=(UV-0.5)*2;
float r=length(p);
float edge=smoothstep(0.965,0.98,r)*(1-smoothstep(0.99,1.0,r));
float a=atan2(p.y,p.x);
float ticks=pow(saturate(cos(a*16)),24)*exp(-pow((r-0.88)/0.014,2));
float fill=0.025*saturate(1-r);
return saturate(edge*(0.55+0.35*saturate(Charge))+ticks*0.3*saturate(Charge)+fill);
''')
    inputs=[]
    for name in ['UV','Charge']:
        item=u.CustomInput()
        item.set_editor_property('input_name',name)
        inputs.append(item)
    shape.set_editor_property('inputs',inputs)
    mel.connect_material_expressions(uv,'',shape,'UV')
    mel.connect_material_expressions(charge,'',shape,'Charge')
    color=mel.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-350,220)
    color.set_editor_property('constant',u.LinearColor(0.025,0.65,1.6,1))
    emission=mel.create_material_expression(mat,u.MaterialExpressionMultiply,-100,0)
    mel.connect_material_expressions(shape,'',emission,'A')
    mel.connect_material_expressions(color,'',emission,'B')
    mel.connect_material_property(emission,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(shape,'',u.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat,only_if_is_dirty=False)
print('범위 재질 저장:',path)

bp=u.load_asset('/Game/Blueprints/Skills/BP_ArcaneOrb')
if not bp:
    raise RuntimeError('BP_ArcaneOrb 없음')
orb=u.get_default_object(bp.generated_class())
try:
    component=orb.get_editor_property('range_mesh')
    orb.get_editor_property('hit_effect')
except Exception:
    print('ORB_RANGE_BUILD_REQUIRED: 새 컴포넌트 빌드 후 스크립트 재실행')
else:
    backup='/Game/Developers/Shared/Backups/BeforeOrbRange/BP_ArcaneOrb'
    if not eal.does_asset_exist(backup):
        if not eal.duplicate_asset(bp.get_path_name(),backup):
            raise RuntimeError('BP 백업 실패')
    component.set_editor_property('static_mesh',u.load_asset('/Engine/BasicShapes/Plane'))
    component.set_material(0,mat)
    component.set_editor_property('relative_location',u.Vector(0,0,-42))
    component.set_editor_property('relative_scale3d',u.Vector(6,6,1))
    orb.set_editor_property('hit_effect',u.load_asset('/Game/Assets/VFX/Mage/NS_MageBolt_Impact'))
    orb.set_editor_property('hit_effect_scale',1.2)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp,only_if_is_dirty=False)
    print('ORB_RANGE_CONNECTED: 반경은 BeginPlay에서 ExplosionRadius로 계산')
