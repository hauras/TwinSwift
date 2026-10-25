import unreal as u
bp=u.load_asset('/Game/UI/WBP_TwinHUD')
tree=u.find_object(None,bp.get_path_name()+':WidgetTree')
expected=['WarriorCard','MageCard','WarriorHealthBar','MageHealthBar','WarriorHealthText','MageHealthText','ActiveName','PassiveText','AssistText']
expected += [f'Skill{i}{suffix}' for i in range(4) for suffix in ['Icon','Cooldown','Name','Key','State']]
for name in expected:
    obj=u.find_object(None,tree.get_path_name()+'.'+name)
    assert obj, 'Missing widget '+name
    print('WIDGET',name,obj.get_class().get_name())
    if isinstance(obj,u.ProgressBar):
        obj.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
root=u.find_object(None,tree.get_path_name()+'.CanvasPanel_0')
# 아이콘은 WBP의 SkillMaterials 칸에서 직접 참조 (Tools/setup_hud_icons.py)
def fix_hit_tests(parent):
    if isinstance(parent,u.PanelWidget):
        for child in parent.get_all_children():
            if isinstance(child,(u.TextBlock,u.ProgressBar,u.Border)):
                child.set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
            elif isinstance(child,u.CanvasPanel):
                child.set_visibility(u.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
            fix_hit_tests(child)
fix_hit_tests(root)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert bp.generated_class()
print('HUD_COMPILED_CLASS',bp.generated_class().get_path_name())
assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
cdo=u.get_default_object(u.load_asset('/Game/Blueprints/Core/BP_PlayerController').generated_class())
assert cdo.get_editor_property('hud_widget_class') == bp.generated_class()
for name in ['WarriorAttack','DashSlash','GroundSlam','Roll','MagicBolt','FrostBolt','Thunder','Blink']:
    assert u.load_asset('/Game/UI/Icons/M_UI_'+name)
print('HUD_VERIFIED: bindings, 4 active skill slots, 8 materials, controller assignment, Blueprint compilation.')
