import unreal as u
E=u.EditorAssetLibrary
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError('PIE 종료 후 적용')
bp=u.load_asset('/Game/Blueprints/Boss/BP_Boss_Conductor')
golem=u.get_default_object(u.load_asset('/Game/Blueprints/Boss/BP_Boss_Golem').generated_class())
source=golem.get_components_by_class(u.SMarkIndicatorComponent)[0]
cdo=u.get_default_object(bp.generated_class())
mark=cdo.get_components_by_class(u.SMarkIndicatorComponent)[0]
for prop in ['crack_icon','frost_icon','widget_class','draw_size','space']:
    value=source.get_editor_property(prop)
    assert value is not None,prop
    mark.set_editor_property(prop,value)
mesh=cdo.get_editor_property('mesh')
bounds=mesh.get_editor_property('skeletal_mesh_asset').get_bounds()
top=mesh.get_editor_property('relative_location').z+(bounds.origin.z+bounds.box_extent.z)*mesh.get_editor_property('relative_scale3d').z
mark.set_editor_property('relative_location',u.Vector(0,0,top+35.0))
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert E.save_loaded_asset(bp)
print('CONDUCTOR_MARKS_FIXED',mark.get_editor_property('crack_icon'),mark.get_editor_property('frost_icon'),mark.get_editor_property('widget_class'),'HEIGHT',top+35.0)
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_class()==bp.generated_class():
        m=a.get_components_by_class(u.SMarkIndicatorComponent)[0]
        print('LEVEL_MARK',m.get_editor_property('crack_icon'),m.get_editor_property('frost_icon'),m.get_editor_property('widget_class'))
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
