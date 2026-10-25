"""WBP_TwinHUD 제작. 명령줄 PythonScriptCommandlet 또는 에디터 Python에서 실행."""
import unreal as u
from pathlib import Path

ASSET='/Game/UI/WBP_TwinHUD'
tools=u.AssetToolsHelpers.get_asset_tools()
eal=u.EditorAssetLibrary
atlas_source=str(Path(__file__).with_name('Art')/'T_UI_SkillAtlas.png')
if not Path(atlas_source).exists():
    atlas_source='D:/Unreal Projects/TwinSwift/Tools/Art/T_UI_SkillAtlas.png'

task=u.AssetImportTask()
task.factory=u.TextureFactory()
task.filename=atlas_source
task.destination_path='/Game/UI/Icons'
task.destination_name='T_UI_SkillAtlas'
task.automated=True
task.replace_existing=True
task.save=True
tools.import_asset_tasks([task])
texture=u.load_asset('/Game/UI/Icons/T_UI_SkillAtlas')
assert texture
texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
eal.save_loaded_asset(texture)

names=['WarriorAttack','DashSlash','GroundSlam','Roll','MagicBolt','FrostBolt','Thunder','Blink']
materials={}
for i,name in enumerate(names):
    path='/Game/UI/Icons/M_UI_'+name
    material=u.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset('M_UI_'+name,'/Game/UI/Icons',u.Material,u.MaterialFactoryNew())
    material.set_editor_property('material_domain',u.MaterialDomain.MD_UI)
    u.MaterialEditingLibrary.delete_all_material_expressions(material)
    def expr(cls,x,y):
        return u.MaterialEditingLibrary.create_material_expression(material,cls,x,y)
    uv=expr(u.MaterialExpressionTextureCoordinate,-600,0)
    mul=expr(u.MaterialExpressionMultiply,-400,0)
    scale=expr(u.MaterialExpressionConstant2Vector,-600,150)
    scale.set_editor_property('r',0.25)
    scale.set_editor_property('g',0.5)
    add=expr(u.MaterialExpressionAdd,-200,0)
    offset=expr(u.MaterialExpressionConstant2Vector,-400,150)
    offset.set_editor_property('r',(i%4)*0.25)
    offset.set_editor_property('g',(i//4)*0.5)
    sample=expr(u.MaterialExpressionTextureSample,0,0)
    sample.set_editor_property('texture',texture)
    sample.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_COLOR)
    for a,ao,b,bi in [(uv,'',mul,'A'),(scale,'',mul,'B'),(mul,'',add,'A'),(offset,'',add,'B'),(add,'',sample,'UVs')]:
        u.MaterialEditingLibrary.connect_material_expressions(a,ao,b,bi)
    u.MaterialEditingLibrary.connect_material_property(sample,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
    u.MaterialEditingLibrary.recompile_material(material)
    eal.save_loaded_asset(material)
    materials[name]=material

if eal.does_asset_exist(ASSET):
    backup='/Game/Developers/Shared/Backups/HUD_BeforeSkillLayout/WBP_TwinHUD'
    if not eal.does_asset_exist(backup): assert eal.duplicate_asset(ASSET,backup)
    bp=u.load_asset(ASSET)
else:
    settings=u.get_default_object(u.load_class(None,'/Script/UMGEditor.UMGEditorProjectSettings'))
    settings.set_editor_property('bUseWidgetTemplateSelector',False)
    settings.set_editor_property('DefaultRootWidget',u.CanvasPanel)
    factory=u.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class',u.STwinHUDWidget)
    bp=tools.create_asset('WBP_TwinHUD','/Game/UI',u.WidgetBlueprint,factory)
tree=u.find_object(None,bp.get_path_name()+':WidgetTree')
root=u.find_object(None,tree.get_path_name()+'.CanvasPanel_0')
if not root:
    root=next((o for o in u.ObjectIterator(u.CanvasPanel) if o.get_outer()==tree),None)
assert isinstance(root,u.CanvasPanel), 'Canvas root required'
root.clear_children()

NAVY=(0.018,0.028,0.048,0.97)
GOLD=(0.4,0.25,0.1,1)
ORANGE=(1,0.45,0.09,1)
BLUE=(0.1,0.6,1,1)
WHITE=(0.93,0.9,0.82,1)
MUTED=(0.48,0.55,0.65,1)
def color(c): return u.LinearColor(*c)
def widget(cls,name): return u.new_object(cls,outer=tree,name=name)
def put(parent,w,x,y,width,height):
    slot=parent.add_child_to_canvas(w)
    slot.set_position(u.Vector2D(x,y))
    slot.set_size(u.Vector2D(width,height))
    return w
def canvas(parent,name,x,y,w,h):
    c=widget(u.CanvasPanel,name)
    c.set_visibility(u.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    return put(parent,c,x,y,w,h)
def rect(parent,name,x,y,w,h,c):
    b=widget(u.Border,name)
    b.set_brush_color(color(c))
    b.set_padding(u.Margin(0))
    b.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return put(parent,b,x,y,w,h)
def text(parent,name,value,x,y,w,h,size=14,c=WHITE):
    t=widget(u.TextBlock,name)
    t.set_text(value)
    f=t.get_editor_property('font')
    f.set_editor_property('size',size)
    t.set_font(f)
    t.set_color_and_opacity(u.SlateColor(specified_color=color(c)))
    t.set_editor_property('justification',u.TextJustify.CENTER)
    t.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return put(parent,t,x,y,w,h)
def image(parent,name,mat,x,y,w,h):
    im=widget(u.Image,name)
    im.set_brush_from_material(materials[mat])
    return put(parent,im,x,y,w,h)
def bar(parent,name,x,y,w,h,c,percent=1):
    b=widget(u.ProgressBar,name)
    b.set_fill_color_and_opacity(color(c))
    b.set_percent(percent)
    b.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
    return put(parent,b,x,y,w,h)

panel=canvas(root,'CombatHUD',0,0,950,226)
slot=panel.get_editor_property('slot')
slot.set_anchors(u.Anchors(minimum=u.Vector2D(0.5,1),maximum=u.Vector2D(0.5,1)))
slot.set_alignment(u.Vector2D(0.5,1))
slot.set_position(u.Vector2D(0,-24))
rect(panel,'PanelFrame',0,0,950,226,GOLD)
rect(panel,'PanelBody',2,2,946,222,NAVY)
rect(panel,'TopAccent',16,6,918,2,GOLD)
rect(panel,'ReserveDivider',720,20,1,184,GOLD)

for role,mat,label,c,health in [('Warrior','DashSlash','전사',ORANGE,'120 / 120'),('Mage','Blink','마법사',BLUE,'90 / 90')]:
    card=canvas(panel,role+'Card',20 if role=='Warrior' else 738,20,174,156)
    card.set_render_transform_pivot(u.Vector2D(0,0))
    rect(card,role+'PortraitFrame',0,0,174,156,c)
    image(card,role+'Portrait',mat,3,3,168,112)
    rect(card,role+'NamePlate',3,90,168,25,NAVY)
    text(card,role+'Label',label,3,92,168,23,15,c)
    bar(card,role+'HealthBar',7,119,160,9,c)
    text(card,role+'HealthText',health,3,133,168,22,13)
    if role=='Mage':
        card.set_render_scale(u.Vector2D(0.75,0.75))
        card.set_render_opacity(0.7)

text(panel,'ActiveName','전사',207,15,480,25,17,ORANGE)
active_names=['대검 콤보','돌진 베기','내려찍기','구르기']
keys=['LMB','1','2','Space']
for i in range(4):
    x=211+i*121
    prefix=f'Skill{i}'
    rect(panel,prefix+'Frame',x,48,102,88,GOLD)
    im=image(panel,prefix+'Icon',names[i],x+2,50,98,84)
    if i>0: im.set_render_opacity(0.3)
    cd=bar(panel,prefix+'Cooldown',x+2,50,98,84,(0.01,0.015,0.025,0.8),0)
    cd.set_editor_property('bar_fill_type',u.ProgressBarFillType.BOTTOM_TO_TOP)
    style=cd.get_editor_property('widget_style')
    background=style.get_editor_property('background_image')
    background.set_editor_property('tint_color',u.SlateColor(specified_color=u.LinearColor(0,0,0,0)))
    style.set_editor_property('background_image',background)
    cd.set_editor_property('widget_style',style)
    text(panel,prefix+'State','' if i==0 else '미구현',x+3,80,96,28,16)
    rect(panel,prefix+'KeyBG',x+21,125,60,24,NAVY)
    text(panel,prefix+'Key',keys[i],x+21,126,60,22,13)
    text(panel,prefix+'Name',active_names[i],x-4,155,110,24,13)
text(panel,'PassiveText','산산조각 · 빙결 적 타격 시 3.0 + 경직 [예정]',203,190,500,22,11,MUTED)
text(panel,'BenchLabel','대기 / 교체',862,26,75,24,12,BLUE)
for i,key in enumerate(['3','4']):
    x=752+i*75
    rect(panel,f'BenchKeyFrame{i}',x,148,60,27,(0.07,0.19,0.29,1))
    text(panel,f'BenchSkill{i+1}Key',key,x,150,60,24,13,BLUE)
text(panel,'AssistText','교체 어시스트: 냉기 장판 [예정]',734,185,205,28,10,MUTED)

# 아이콘은 WBP의 SkillMaterials 칸에서 직접 참조 (Tools/setup_hud_icons.py)

u.BlueprintEditorLibrary.compile_blueprint(bp)
assert eal.save_loaded_asset(bp,only_if_is_dirty=False)
controller=u.load_asset('/Game/Blueprints/Core/BP_PlayerController')
backup='/Game/Developers/Shared/Backups/HUD_BeforeSkillLayout/BP_PlayerController'
if not eal.does_asset_exist(backup): assert eal.duplicate_asset('/Game/Blueprints/Core/BP_PlayerController',backup)
cdo=u.get_default_object(controller.generated_class())
cdo.set_editor_property('hud_widget_class',bp.generated_class())
u.BlueprintEditorLibrary.compile_blueprint(controller)
assert eal.save_loaded_asset(controller,only_if_is_dirty=False)
print('HUD_CREATED',ASSET,'ICON_MATERIALS',len(materials),'CONTROLLER_LINKED',bp.generated_class())
