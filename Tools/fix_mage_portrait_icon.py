"""순간이동 스킬과 캐릭터 초상화를 분리하고 초상화 종횡비를 유지한다."""
import unreal as u

eal=u.EditorAssetLibrary
mel=u.MaterialEditingLibrary
assert not u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
root='/Game/UI/Icons/LowPoly'
texture=u.load_asset(root+'/T_UI_LowPolySkillAtlas')
assert texture
bp=u.load_asset('/Game/UI/WBP_TwinHUD')
assert bp
backup='/Game/Developers/Shared/Backups/HUD_BeforeMagePortraitFix_20261106/WBP_TwinHUD'
if not eal.does_asset_exist(backup):
    assert eal.duplicate_asset('/Game/UI/WBP_TwinHUD',backup)
    assert eal.save_asset(backup,only_if_is_dirty=False)
name='M_UI_LP_MagePortrait'
path=root+'/'+name
mat=u.load_asset(path) if eal.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,root,u.Material,u.MaterialFactoryNew())
assert mat
mat.set_editor_property('material_domain',u.MaterialDomain.MD_UI)
mat.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE)
mel.delete_all_material_expressions(mat)
def node(cls,x,y):
    return mel.create_material_expression(mat,cls,x,y)
def constant2(x,y,a,b):
    n=node(u.MaterialExpressionConstant2Vector,x,y)
    n.set_editor_property('r',a)
    n.set_editor_property('g',b)
    return n
def connect(a,b,pin):
    assert mel.connect_material_expressions(a,'',b,pin)

# 아틀라스의 왼쪽 마법사 한 명만 선택한다. 스킬용 Blink 영역은 바꾸지 않는다.
x0,y0,x1,y1=1223,423,1378,612
uv=node(u.MaterialExpressionTextureCoordinate,-1100,0)
atlas_scale=constant2(-430,180,(x1-x0)/1983.0,(y1-y0)/793.0)
atlas_mul=node(u.MaterialExpressionMultiply,-200,0)
connect(uv,atlas_mul,'A')
connect(atlas_scale,atlas_mul,'B')
atlas_offset=constant2(-200,180,x0/1983.0,y0/793.0)
atlas_add=node(u.MaterialExpressionAdd,30,0)
connect(atlas_mul,atlas_add,'A')
connect(atlas_offset,atlas_add,'B')
sample=node(u.MaterialExpressionTextureSample,260,0)
sample.set_editor_property('texture',texture)
sample.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_COLOR)
connect(atlas_add,sample,'UVs')
assert mel.connect_material_property(sample,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(mat)
assert eal.save_loaded_asset(mat,only_if_is_dirty=False)

tree=u.find_object(None,bp.get_path_name()+':WidgetTree')
image=u.find_object(None,tree.get_path_name()+'.MagePortrait')
assert image
image.set_brush_from_material(mat)
# 그림 칸 자체를 원본 비율로 맞추고 카드 가운데 둔다.
portrait_width=112.0*(x1-x0)/(y1-y0)
slot=image.get_editor_property('slot')
slot.set_position(u.Vector2D((174.0-portrait_width)*.5,3.0))
slot.set_size(u.Vector2D(portrait_width,112.0))
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert eal.save_loaded_asset(bp,only_if_is_dirty=False)
assert image.get_editor_property('brush').get_editor_property('resource_object')==mat
icons=u.get_default_object(bp.generated_class()).get_editor_property('skill_materials')
assert icons[u.Name('Blink')].get_name()=='M_UI_LP_Blink'
assert icons[u.Name('ArcaneOrb')].get_name()=='M_UI_LP_ArcaneOrb'
print('MAGE_PORTRAIT_FIXED',mat.get_path_name(),'single character; 168x112 aspect preserved; Blink skill unchanged')
