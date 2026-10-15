import unreal as u
sub=u.get_editor_subsystem(u.UnrealEditorSubsystem)
w=sub.get_editor_world()
floor=u.EditorLevelLibrary.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,-15))
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
floor.set_actor_scale3d(u.Vector(10,10,0.2))
sun=u.EditorLevelLibrary.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,500),u.Rotator(-50,-30,0))
sun.get_component_by_class(u.DirectionalLightComponent).set_intensity(3)
for role,x in [('ToMage',-145),('ToWarrior',145)]:
    system=u.load_asset('/Game/VFX/Tag/NS_TagSwap_'+role)
    report=u.TSWeaponTrailLibrary.tune_trail_timing(system,1,1)
    assert 'VALID=1' in report
    assert 'InitializeParticle' not in report, 'Unexpected initializer can overwrite color'
    comp=u.NiagaraFunctionLibrary.spawn_system_at_location(w,system,u.Vector(x,0,0),auto_destroy=False,auto_activate=False,pre_cull_check=False)
    comp.set_force_solo(True)
    comp.activate(True)
    comp.advance_simulation(10,0.01)
    comp.set_paused(True)
    print('FROZEN_COLORED_EFFECT',role,comp.get_path_name())
u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(0,-650,800),u.Rotator(-50,90,0))
u.SystemLibrary.execute_console_command(w,'r.EyeAdaptationQuality 0')
print('COLOR_PREVIEW_READY: left blue arrival, right orange arrival, paused at 0.10 sec; preview world not saved.')
