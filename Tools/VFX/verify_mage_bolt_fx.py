import unreal as u
for role in ['Flight','Launch','Impact']:
    asset=u.load_asset('/Game/VFX/Mage/NS_MageBolt_'+role)
    assert asset
    report=u.TSWeaponTrailLibrary.tune_trail_timing(asset,1,1)
    assert 'VALID=1' in report, report
    assert report.count('EMITTER ')==3, report
    assert 'InitializeParticle' not in report, 'Initializer may overwrite color'
    print('MAGE_BOLT_VERIFY',role,report)
for name in ['Orb','Tail','Ring','Spark']:
    m=u.load_asset('/Game/VFX/Mage/M_MageBolt_'+name)
    assert m and m.get_editor_property('used_with_niagara_sprites')
    assert m.get_editor_property('blend_mode')==u.BlendMode.BLEND_ADDITIVE
seq=u.load_asset('/Game/Asset/Wizard_Free/Animations/A_Wizard_1HMagicAttack_01')
assert seq
print('CAST_ANIMATION',seq.get_play_length(),seq.get_editor_property('skeleton').get_path_name())
print('MAGE_BOLT_ASSETS_VERIFIED')
