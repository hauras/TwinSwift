"""승인된 시안을 UI 아틀라스로 임포트하고 HUD 그림만 교체한다."""
import unreal as u
from pathlib import Path
import json

ROOT = '/Game/UI/Icons/LowPoly'
SOURCE = 'D:/Unreal Projects/TwinSwift/SourceAssets/UI/SkillIcons/T_UI_LowPolySkillAtlas.png'
BACKUP = '/Game/Developers/Shared/Backups/HUD_BeforeLowPolyIcons_20261106/WBP_TwinHUD'
WBP = '/Game/UI/WBP_TwinHUD'
SIZE = (1983, 793)
NAMES = ['WarriorAttack', 'DashSlash', 'GroundSlam', 'Roll', 'Parry', 'MagicBolt', 'FrostBolt', 'Thunder', 'Blink', 'ArcaneOrb']
COLUMNS = [(28,386),(421,778),(814,1172),(1207,1563),(1599,1955)]
ROWS = [(25,377),(414,765)]

ed = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert not ed.get_game_world(), 'PIE 중에는 HUD 에셋을 수정하지 않는다'
assert Path(SOURCE).is_file()
eal = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
mel = u.MaterialEditingLibrary
if not eal.does_asset_exist(BACKUP):
    assert eal.duplicate_asset(WBP, BACKUP)
    assert eal.save_asset(BACKUP, only_if_is_dirty=False)

task = u.AssetImportTask()
task.set_editor_property('factory', u.TextureFactory())
task.set_editor_property('filename', SOURCE)
task.set_editor_property('destination_path', ROOT)
task.set_editor_property('destination_name', 'T_UI_LowPolySkillAtlas')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
tools.import_asset_tasks([task])
texture = u.load_asset(ROOT + '/T_UI_LowPolySkillAtlas')
assert texture
texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('mip_gen_settings', u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('lod_group', u.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('address_x', u.TextureAddress.TA_CLAMP)
texture.set_editor_property('address_y', u.TextureAddress.TA_CLAMP)
texture.set_editor_property('srgb', True)
assert eal.save_loaded_asset(texture, only_if_is_dirty=False)

materials = {}
regions = {}
for index, name in enumerate(NAMES):
    col, row = index % 5, index // 5
    x0, x1 = COLUMNS[col]
    y0, y1 = ROWS[row]
    regions[name] = [x0, y0, x1, y1]
    path = ROOT + '/M_UI_LP_' + name
    material = u.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset('M_UI_LP_' + name, ROOT, u.Material, u.MaterialFactoryNew())
    assert material
    material.set_editor_property('material_domain', u.MaterialDomain.MD_UI)
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_OPAQUE)
    mel.delete_all_material_expressions(material)
    uv = mel.create_material_expression(material, u.MaterialExpressionTextureCoordinate, -700, 0)
    scale = mel.create_material_expression(material, u.MaterialExpressionConstant2Vector, -700, 160)
    scale.set_editor_property('r', (x1-x0)/SIZE[0])
    scale.set_editor_property('g', (y1-y0)/SIZE[1])
    multiply = mel.create_material_expression(material, u.MaterialExpressionMultiply, -450, 0)
    offset = mel.create_material_expression(material, u.MaterialExpressionConstant2Vector, -450, 160)
    offset.set_editor_property('r', x0/SIZE[0])
    offset.set_editor_property('g', y0/SIZE[1])
    add = mel.create_material_expression(material, u.MaterialExpressionAdd, -200, 0)
    sample = mel.create_material_expression(material, u.MaterialExpressionTextureSample, 40, 0)
    sample.set_editor_property('texture', texture)
    sample.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_COLOR)
    for src, output, dst, pin in [(uv,'',multiply,'A'),(scale,'',multiply,'B'),(multiply,'',add,'A'),(offset,'',add,'B'),(add,'',sample,'UVs')]:
        assert mel.connect_material_expressions(src,output,dst,pin)
    assert mel.connect_material_property(sample, 'RGB', u.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    assert eal.save_loaded_asset(material, only_if_is_dirty=False)
    materials[name] = material

bp = u.load_asset(WBP)
assert bp and bp.generated_class()
cdo = u.get_default_object(bp.generated_class())
slots = dict(cdo.get_editor_property('skill_materials'))
for name, material in materials.items():
    slots[u.Name(name)] = material
cdo.set_editor_property('skill_materials', slots)
tree = u.find_object(None, bp.get_path_name() + ':WidgetTree')
assert tree
def widget(name):
    result = u.find_object(None, tree.get_path_name() + '.' + name)
    assert result, '위젯 없음: ' + name
    return result

for index, name in enumerate(NAMES[:5]):
    widget('Skill%dIcon' % index).set_brush_from_material(materials[name])
widget('WarriorPortrait').set_brush_from_material(materials['DashSlash'])
mage_portrait = u.load_asset(ROOT + '/M_UI_LP_MagePortrait') if eal.does_asset_exist(ROOT + '/M_UI_LP_MagePortrait') else materials['Blink']
widget('MagePortrait').set_brush_from_material(mage_portrait)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert eal.save_loaded_asset(bp, only_if_is_dirty=False)

saved = u.get_default_object(bp.generated_class()).get_editor_property('skill_materials')
for name in NAMES:
    assert saved[u.Name(name)].get_path_name() == materials[name].get_path_name()
assert len(NAMES) == 10
Path('D:/Unreal Projects/TwinSwift/SourceAssets/UI/SkillIcons/atlas_regions.json').write_text(json.dumps({'image':'T_UI_LowPolySkillAtlas.png','size':SIZE,'regions':regions},ensure_ascii=False,indent=2),encoding='utf-8')
print('LOWPOLY_SKILL_ICONS_SAVED', {name:material.get_path_name() for name,material in materials.items()})
print('LOWPOLY_HUD_BACKUP', BACKUP)
