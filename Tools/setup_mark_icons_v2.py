"""전용 표식 아이콘, 디자이너 WBP 및 적 BP 연결. 기존 실행 에디터에서 실행."""
import unreal as u
from pathlib import Path

tools = u.AssetToolsHelpers.get_asset_tools()
eal = u.EditorAssetLibrary
lib = u.MaterialEditingLibrary
materials = {}
for kind, filename in [('Crack', 'T_Mark_Fracture'), ('Frost', 'T_Mark_Frost')]:
    task = u.AssetImportTask()
    task.filename = str(Path('D:/Unreal Projects/TwinSwift/Tools/Art/Marks') / (filename + '.png'))
    task.destination_path = '/Game/UI/Marks'
    task.destination_name = filename
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    tex = u.load_asset('/Game/UI/Marks/' + filename)
    assert tex
    tex.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property('mip_gen_settings', u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property('lod_group', u.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property('srgb', True)
    eal.save_loaded_asset(tex)
    name = 'M_UI_Mark_' + kind
    mat = u.load_asset('/Game/UI/Marks/' + name)
    if not mat:
        mat = tools.create_asset(name, '/Game/UI/Marks', u.Material, u.MaterialFactoryNew())
    mat.set_editor_property('material_domain', u.MaterialDomain.MD_UI)
    mat.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
    lib.delete_all_material_expressions(mat)
    sample = lib.create_material_expression(mat, u.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property('texture', tex)
    sample.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_COLOR)
    lib.connect_material_property(sample, 'RGB', u.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.connect_material_property(sample, 'A', u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert eal.save_loaded_asset(mat)
    materials[kind] = mat

path = '/Game/UI/WBP_MarkIndicator'
bp = u.load_asset(path)
if not bp:
    settings = u.get_default_object(u.load_class(None, '/Script/UMGEditor.UMGEditorProjectSettings'))
    old_selector = settings.get_editor_property('bUseWidgetTemplateSelector')
    old_root = settings.get_editor_property('DefaultRootWidget')
    try:
        settings.set_editor_property('bUseWidgetTemplateSelector', False)
        settings.set_editor_property('DefaultRootWidget', u.CanvasPanel)
        factory = u.WidgetBlueprintFactory()
        factory.set_editor_property('parent_class', u.SMarkIconWidget)
        bp = tools.create_asset('WBP_MarkIndicator', '/Game/UI', u.WidgetBlueprint, factory)
    finally:
        settings.set_editor_property('bUseWidgetTemplateSelector', old_selector)
        settings.set_editor_property('DefaultRootWidget', old_root)
assert bp
tree = u.find_object(None, bp.get_path_name() + ':WidgetTree')
root = next(o for o in u.ObjectIterator(u.CanvasPanel) if o.get_outer() == tree)
root.clear_children()
row = u.new_object(u.HorizontalBox, outer=tree, name='MarkRow')
slot = root.add_child_to_canvas(row)
slot.set_anchors(u.Anchors(u.Vector2D(0.5, 0.5), u.Vector2D(0.5, 0.5)))
slot.set_alignment(u.Vector2D(0.5, 0.5))
slot.set_auto_size(True)
slot.set_position(u.Vector2D(0, 0))
for kind, color in [('Crack', (1, .45, .1, 1)), ('Frost', (.15, .75, 1, 1))]:
    box = u.new_object(u.SizeBox, outer=tree, name=kind + 'Root')
    box.set_width_override(36)
    rs = row.add_child_to_horizontal_box(box)
    rs.set_padding(u.Margin(2, 0, 2, 0))
    col = u.new_object(u.VerticalBox, outer=tree, name=kind + 'Column')
    box.add_child(col)
    icon = u.new_object(u.Image, outer=tree, name=kind + 'Icon')
    icon.set_brush_from_material(materials[kind])
    icon.set_desired_size_override(u.Vector2D(36, 36))
    col.add_child_to_vertical_box(icon).set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_CENTER)
    bar_box = u.new_object(u.SizeBox, outer=tree, name=kind + 'BarSize')
    bar_box.set_height_override(3)
    bar = u.new_object(u.ProgressBar, outer=tree, name=kind + 'TimeBar')
    bar.set_fill_color_and_opacity(u.LinearColor(*color))
    bar.set_percent(1)
    style = bar.get_editor_property('widget_style')
    for field, tint in [('background_image', (.015, .02, .035, .9)), ('fill_image', (1, 1, 1, 1))]:
        brush = style.get_editor_property(field)
        brush.set_editor_property('draw_as', u.SlateBrushDrawType.IMAGE)
        brush.set_editor_property('tint_color', u.SlateColor(specified_color=u.LinearColor(*tint)))
        style.set_editor_property(field, brush)
    bar.set_editor_property('widget_style', style)
    bar_box.add_child(bar)
    col.add_child_to_vertical_box(bar_box).set_padding(u.Margin(3, 2, 3, 0))
root.set_visibility(u.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
u.TSWidgetEditLibrary.mark_modified(bp)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert bp.generated_class()
assert eal.save_loaded_asset(bp, only_if_is_dirty=False)

enemy = u.load_asset('/Game/Blueprints/Enemy/BP_Dummy')
indicator = u.get_default_object(enemy.generated_class()).get_editor_property('mark_indicator')
indicator.set_editor_property('crack_icon', materials['Crack'])
indicator.set_editor_property('frost_icon', materials['Frost'])
indicator.set_editor_property('widget_class', bp.generated_class())
u.BlueprintEditorLibrary.compile_blueprint(enemy)
assert eal.save_loaded_asset(enemy, only_if_is_dirty=False)
print('MARK_DESIGNER_TREE', u.TSWidgetEditLibrary.dump_widget_tree(bp))
hud = u.load_asset('/Game/UI/WBP_TwinHUD')
print('EXISTING_HUD_TREE', u.TSWidgetEditLibrary.dump_widget_tree(hud))
assert indicator.get_editor_property('crack_icon') == materials['Crack']
assert indicator.get_editor_property('widget_class') == bp.generated_class()
print('MARK_UI_ASSETS_VERIFIED: 2 textures, alpha materials, WBP, enemy assignment. C++ rebuild required.')
